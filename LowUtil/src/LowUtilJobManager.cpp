#include "LowUtilJobManager.h"
#include "LowUtilHandle.h"
#include "LowUtilHashing.h"
#include "LowUtilLogger.h"
#include "LowUtilSerialization.h"
#include "LowMath.h"

#include <fstream>
#include <string>
#include <chrono>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace Low {
  namespace Util {
    namespace JobManager {

      // -----------------------------------------------------------------------
      // Legacy ThreadPool
      // -----------------------------------------------------------------------

      ThreadPool *g_DefaultThreadPool;

#if defined(_WIN32)
      static std::wstring to_wstring(const char *p_Str)
      {
        if (!p_Str) {
          return {};
        }
        int l_Size =
            MultiByteToWideChar(CP_UTF8, 0, p_Str, -1, nullptr, 0);
        if (l_Size <= 0) {
          return {};
        }
        std::wstring l_Wide(l_Size - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, p_Str, -1, &l_Wide[0],
                            l_Size);
        return l_Wide;
      }

      static void set_thread_name(std::thread &p_Thread,
                                  const char *p_Name)
      {
        SetThreadDescription(p_Thread.native_handle(),
                             to_wstring(p_Name).c_str());
      }
#else
      static void set_thread_name(std::thread &, const char *)
      {
      }
#endif

      ThreadPool::ThreadPool(int p_NumThreads) : m_Stop(false)
      {
        for (int i = 0; i < p_NumThreads; ++i) {
          Util::String i_Name = "Worker ";
          i_Name += std::to_string(i + 1).c_str();

          m_Workers.emplace_back([this, i_Name] {
            Log::set_current_thread_name(i_Name.c_str());

            while (true) {
              Function<void()> l_Task;
              {
                std::unique_lock<std::mutex> l_Lock(m_QueueMutex);
                m_Condition.wait(l_Lock, [this] {
                  return m_Stop || !m_JobQueue.empty();
                });
                if (m_Stop && m_JobQueue.empty()) {
                  return;
                }
                l_Task = std::move(m_JobQueue.front());
                m_JobQueue.pop();
              }
              l_Task();
            }
          });

          set_thread_name(m_Workers[i], i_Name.c_str());
        }
      }

      ThreadPool::~ThreadPool()
      {
        {
          std::unique_lock<std::mutex> l_Lock(m_QueueMutex);
          m_Stop = true;
        }
        m_Condition.notify_all();
        for (std::thread &i_Worker : m_Workers) {
          i_Worker.join();
        }
      }

      void initialize()
      {
        g_DefaultThreadPool = new ThreadPool(4);
        IO::initialize();
        Background::initialize();
      }

      void cleanup()
      {
        Background::cleanup();
        IO::cleanup();
        delete g_DefaultThreadPool;
      }

      ThreadPool &default_pool()
      {
        return *g_DefaultThreadPool;
      }

      // -----------------------------------------------------------------------
      // Job Tracking
      // -----------------------------------------------------------------------

      namespace {
#if LOW_JOBMANAGER_TRACKING
        struct TrackedJob
        {
          u64 id = 0;
          Tracking::JobType type = Tracking::JobType::Background;
          std::atomic<int> status{
              static_cast<int>(Background::JobStatus::Pending)};
          std::atomic<float> progress{0.0f};
          std::atomic<u64> elapsedMs{0};
          std::chrono::steady_clock::time_point startTime;
          String label;
          String detail;
        };

        static std::atomic<u64> g_NextTrackedJobId{1};
        static UnorderedMap<u64, SharedPtr<TrackedJob>> g_TrackedJobs;
        static std::mutex g_TrackedJobsMutex;

        static u64 tracking_begin(Tracking::JobType p_Type,
                                  const char *p_Label,
                                  const String &p_Detail)
        {
          SharedPtr<TrackedJob> l_Job = make_shared<TrackedJob>();
          l_Job->id = g_NextTrackedJobId.fetch_add(
              1, std::memory_order_relaxed);
          l_Job->type = p_Type;
          l_Job->label = p_Label;
          l_Job->detail = p_Detail;
          l_Job->startTime = std::chrono::steady_clock::now();

          std::unique_lock<std::mutex> l_Lock(g_TrackedJobsMutex);
          g_TrackedJobs[l_Job->id] = l_Job;
          return l_Job->id;
        }

        static u64 tracking_begin(Tracking::JobType p_Type,
                                  const String &p_Label,
                                  const String &p_Detail)
        {
          SharedPtr<TrackedJob> l_Job = make_shared<TrackedJob>();
          l_Job->id = g_NextTrackedJobId.fetch_add(
              1, std::memory_order_relaxed);
          l_Job->type = p_Type;
          l_Job->label = p_Label;
          l_Job->detail = p_Detail;
          l_Job->startTime = std::chrono::steady_clock::now();

          std::unique_lock<std::mutex> l_Lock(g_TrackedJobsMutex);
          g_TrackedJobs[l_Job->id] = l_Job;
          return l_Job->id;
        }

        static u64 tracking_begin(Tracking::JobType p_Type,
                                  const String &p_Label,
                                  const char *p_Detail)
        {
          SharedPtr<TrackedJob> l_Job = make_shared<TrackedJob>();
          l_Job->id = g_NextTrackedJobId.fetch_add(
              1, std::memory_order_relaxed);
          l_Job->type = p_Type;
          l_Job->label = p_Label;
          l_Job->detail = p_Detail;
          l_Job->startTime = std::chrono::steady_clock::now();

          std::unique_lock<std::mutex> l_Lock(g_TrackedJobsMutex);
          g_TrackedJobs[l_Job->id] = l_Job;
          return l_Job->id;
        }

        static void
        tracking_set_status(u64 p_Id, Background::JobStatus p_Status)
        {
          std::unique_lock<std::mutex> l_Lock(g_TrackedJobsMutex);
          auto l_It = g_TrackedJobs.find(p_Id);
          if (l_It == g_TrackedJobs.end()) {
            return;
          }
          l_It->second->status.store(static_cast<int>(p_Status),
                                     std::memory_order_release);
        }

        static void tracking_set_progress(u64 p_Id, float p_Progress)
        {
          std::unique_lock<std::mutex> l_Lock(g_TrackedJobsMutex);
          auto l_It = g_TrackedJobs.find(p_Id);
          if (l_It == g_TrackedJobs.end()) {
            return;
          }
          l_It->second->progress.store(p_Progress,
                                       std::memory_order_relaxed);
        }

        static void tracking_finish(u64 p_Id, bool p_Success)
        {
          std::unique_lock<std::mutex> l_Lock(g_TrackedJobsMutex);
          auto l_It = g_TrackedJobs.find(p_Id);
          if (l_It == g_TrackedJobs.end()) {
            return;
          }
          const auto l_Now = std::chrono::steady_clock::now();
          l_It->second->elapsedMs.store(
              static_cast<u64>(std::chrono::duration_cast<
                                   std::chrono::milliseconds>(
                                   l_Now - l_It->second->startTime)
                                   .count()),
              std::memory_order_relaxed);
          l_It->second->progress.store(p_Success ? 1.0f : 0.0f,
                                       std::memory_order_relaxed);
          l_It->second->status.store(
              static_cast<int>(p_Success
                                   ? Background::JobStatus::Completed
                                   : Background::JobStatus::Failed),
              std::memory_order_release);
        }

        static void tracking_release(u64 p_Id)
        {
          std::unique_lock<std::mutex> l_Lock(g_TrackedJobsMutex);
          g_TrackedJobs.erase(p_Id);
        }
#else
        static u64 tracking_begin(Tracking::JobType, const char *,
                                  const String &)
        {
          return 0;
        }

        static u64 tracking_begin(Tracking::JobType, const String &,
                                  const String &)
        {
          return 0;
        }

        static u64 tracking_begin(Tracking::JobType, const String &,
                                  const char *)
        {
          return 0;
        }

        static void tracking_set_status(u64, Background::JobStatus)
        {
        }

        static void tracking_set_progress(u64, float)
        {
        }

        static void tracking_finish(u64, bool)
        {
        }

        static void tracking_release(u64)
        {
        }
#endif
      } // namespace

      // -----------------------------------------------------------------------
      // IO Worker
      // -----------------------------------------------------------------------

      namespace IO {

        static std::thread g_IoThread;
        static Queue<Function<void()>> g_IoQueue;
        static std::mutex g_IoQueueMutex;
        static std::condition_variable g_IoCondition;
        static bool g_IoStop = false;

        static List<Function<void()>> g_IoCompleted;
        static std::mutex g_IoCompletedMutex;

        static void io_post_completion(Function<void()> p_Fn)
        {
          std::unique_lock<std::mutex> l_Lock(g_IoCompletedMutex);
          g_IoCompleted.push_back(std::move(p_Fn));
        }

        static void io_enqueue(Function<void()> p_Work)
        {
          {
            std::unique_lock<std::mutex> l_Lock(g_IoQueueMutex);
            g_IoQueue.push(std::move(p_Work));
          }
          g_IoCondition.notify_one();
        }

        static void io_worker_func()
        {
          while (true) {
            Function<void()> l_Work;
            {
              std::unique_lock<std::mutex> l_Lock(g_IoQueueMutex);
              g_IoCondition.wait(l_Lock, [] {
                return g_IoStop || !g_IoQueue.empty();
              });
              if (g_IoStop && g_IoQueue.empty()) {
                return;
              }
              l_Work = std::move(g_IoQueue.front());
              g_IoQueue.pop();
            }
            l_Work();
          }
        }

        void initialize()
        {
          g_IoStop = false;
          g_IoThread = std::thread([] {
            Log::set_current_thread_name("IO Worker");
            io_worker_func();
          });
          set_thread_name(g_IoThread, "IO Worker");
        }

        void cleanup()
        {
          {
            std::unique_lock<std::mutex> l_Lock(g_IoQueueMutex);
            g_IoStop = true;
          }
          g_IoCondition.notify_one();
          g_IoThread.join();
        }

        void flush_callbacks()
        {
          List<Function<void()>> l_Batch;
          {
            std::unique_lock<std::mutex> l_Lock(g_IoCompletedMutex);
            l_Batch.swap(g_IoCompleted);
          }
          for (auto &i_Fn : l_Batch) {
            i_Fn();
          }
        }

        void schedule_read_raw(
            String p_Path,
            Function<void(bool, List<uint8_t> &)> p_Callback)
        {
          u64 l_JobId = tracking_begin(Tracking::JobType::IO,
                                       "Read raw", p_Path);
          io_enqueue([l_Path = std::move(p_Path),
                      l_Callback = std::move(p_Callback),
                      l_JobId]() mutable {
            tracking_set_status(l_JobId,
                                Background::JobStatus::Running);
            bool l_Success = false;
            List<uint8_t> l_Data;
            try {
              std::ifstream l_File(l_Path.c_str(),
                                   std::ios::binary | std::ios::ate);
              if (l_File.is_open()) {
                std::streamsize l_Size = l_File.tellg();
                l_File.seekg(0, std::ios::beg);
                l_Data.resize(static_cast<size_t>(l_Size));
                l_File.read(reinterpret_cast<char *>(l_Data.data()),
                            l_Size);
                l_Success = l_File.good();
              }
            } catch (...) {
            }
            io_post_completion([l_Success, l_Data = std::move(l_Data),
                                l_Callback = std::move(l_Callback),
                                l_JobId]() mutable {
              l_Callback(l_Success, l_Data);
              tracking_finish(l_JobId, l_Success);
            });
          });
        }

        void schedule_read_texture(
            String p_Path,
            Function<void(bool, Resource::ImageMipMaps &)> p_Callback)
        {
          u64 l_JobId = tracking_begin(Tracking::JobType::IO,
                                       "Read texture", p_Path);
          io_enqueue([l_Path = std::move(p_Path),
                      l_Callback = std::move(p_Callback),
                      l_JobId]() mutable {
            tracking_set_status(l_JobId,
                                Background::JobStatus::Running);
            bool l_Success = false;
            Resource::ImageMipMaps l_MipMaps;
            try {
              Resource::load_image_mipmaps(l_Path, l_MipMaps);
              l_Success = true;
            } catch (...) {
            }
            io_post_completion([l_Success,
                                l_MipMaps = std::move(l_MipMaps),
                                l_Callback = std::move(l_Callback),
                                l_JobId]() mutable {
              l_Callback(l_Success, l_MipMaps);
              tracking_finish(l_JobId, l_Success);
            });
          });
        }

        void schedule_read_mesh(
            String p_MeshPath, String p_SidecarPath,
            Function<void(bool, MeshLoadResult &)> p_Callback)
        {
          u64 l_JobId = tracking_begin(Tracking::JobType::IO,
                                       "Read mesh", p_MeshPath);
          io_enqueue([l_MeshPath = std::move(p_MeshPath),
                      l_SidecarPath = std::move(p_SidecarPath),
                      l_Callback = std::move(p_Callback),
                      l_JobId]() mutable {
            tracking_set_status(l_JobId,
                                Background::JobStatus::Running);
            bool l_Success = false;
            MeshLoadResult l_Result;
            try {
              Resource::load_mesh(l_MeshPath, l_Result.mesh);

              Serial::Node l_Sidecar =
                  Serial::load_yaml_file(l_SidecarPath.c_str());

              l_Result.aabb = l_Sidecar["aabb"].as<Math::AABB>();

              if (l_Sidecar["bounding_sphere"]) {
                l_Result.bounding_sphere =
                    l_Sidecar["bounding_sphere"].as<Math::Sphere>();
              }

              l_Result.skeleton_id = Util::Handle::DEAD;
              if (l_Sidecar["skeleton"]) {
                l_Result.skeleton_id =
                    l_Sidecar["skeleton"].as<Util::U64Id>();
              }

              for (const auto &i_Submesh : l_Result.mesh.submeshes) {
                for (const auto &i_MeshInfo : i_Submesh.meshInfos) {
                  const char *i_NameBuf = i_MeshInfo.name.c_str();
                  if (l_Sidecar["submeshes"][i_NameBuf]) {
                    l_Result.submesh_aabbs[i_MeshInfo.name] =
                        l_Sidecar["submeshes"][i_NameBuf]["aabb"]
                            .as<Math::AABB>();
                    l_Result
                        .submesh_bounding_spheres[i_MeshInfo.name] =
                        l_Sidecar["submeshes"][i_NameBuf]
                                 ["bounding_sphere"]
                                     .as<Math::Sphere>();
                  }
                }
              }

              l_Success = true;
            } catch (...) {
            }
            io_post_completion([l_Success,
                                l_Result = std::move(l_Result),
                                l_Callback = std::move(l_Callback),
                                l_JobId]() mutable {
              l_Callback(l_Success, l_Result);
              tracking_finish(l_JobId, l_Success);
            });
          });
        }

        void schedule_read_yaml(
            String p_Path,
            Function<void(bool, Serial::Node &)> p_Callback)
        {
          u64 l_JobId = tracking_begin(Tracking::JobType::IO,
                                       "Read yaml", p_Path);
          io_enqueue([l_Path = std::move(p_Path),
                      l_Callback = std::move(p_Callback),
                      l_JobId]() mutable {
            tracking_set_status(l_JobId,
                                Background::JobStatus::Running);
            bool l_Success = false;
            Serial::Node l_Node;
            try {
              l_Node = Serial::load_yaml_file(l_Path.c_str());
              l_Success = true;
            } catch (...) {
            }
            io_post_completion([l_Success, l_Node = std::move(l_Node),
                                l_Callback = std::move(l_Callback),
                                l_JobId]() mutable {
              l_Callback(l_Success, l_Node);
              tracking_finish(l_JobId, l_Success);
            });
          });
        }

        void schedule_write_yaml(String p_Path, Serial::Node p_Node,
                                 Function<void(bool)> p_Callback)
        {
          u64 l_JobId = tracking_begin(Tracking::JobType::IO,
                                       "Write yaml", p_Path);
          io_enqueue([l_Path = std::move(p_Path),
                      l_Node = std::move(p_Node),
                      l_Callback = std::move(p_Callback),
                      l_JobId]() mutable {
            tracking_set_status(l_JobId,
                                Background::JobStatus::Running);
            bool l_Success = false;
            try {
              Serial::write_yaml_file(l_Path.c_str(), l_Node);
              l_Success = true;
            } catch (...) {
            }
            if (l_Callback) {
              io_post_completion([l_Success,
                                  l_Callback = std::move(l_Callback),
                                  l_JobId]() mutable {
                l_Callback(l_Success);
                tracking_finish(l_JobId, l_Success);
              });
            } else {
              tracking_finish(l_JobId, l_Success);
            }
          });
        }

      } // namespace IO

      // -----------------------------------------------------------------------
      // Background Workers
      // -----------------------------------------------------------------------

      namespace Background {

        struct BackgroundJobEntry
        {
          u64 id;
          u64 trackedId = 0;
          JobPriority priority;
          std::atomic<int> status{
              static_cast<int>(JobStatus::Pending)};
          std::atomic<float> progress{0.0f};
          String label;
          Function<void(Function<void(float)>)> work;
        };

        struct BackgroundQueueItem
        {
          JobPriority priority;
          u64 id;

          bool operator<(const BackgroundQueueItem &p_Other) const
          {
            return static_cast<int>(priority) <
                   static_cast<int>(p_Other.priority);
          }
        };

        static std::atomic<u64> g_NextJobId{1};
        static UnorderedMap<u64, SharedPtr<BackgroundJobEntry>>
            g_Jobs;
        static PriorityQueue<BackgroundQueueItem> g_Queue;
        static std::mutex g_Mutex;
        static std::condition_variable g_Condition;
        static bool g_Stop = false;
        static List<std::thread> g_Workers;

        static void worker_func()
        {
          while (true) {
            SharedPtr<BackgroundJobEntry> l_Entry;
            {
              std::unique_lock<std::mutex> l_Lock(g_Mutex);
              g_Condition.wait(
                  l_Lock, [] { return g_Stop || !g_Queue.empty(); });
              if (g_Stop && g_Queue.empty()) {
                return;
              }

              u64 l_Id = g_Queue.top().id;
              g_Queue.pop();

              auto l_It = g_Jobs.find(l_Id);
              if (l_It == g_Jobs.end()) {
                continue;
              }
              l_Entry = l_It->second;
              l_Entry->status.store(
                  static_cast<int>(JobStatus::Running),
                  std::memory_order_release);
              tracking_set_status(l_Entry->trackedId,
                                  JobStatus::Running);
            }

            try {
              l_Entry->work([&l_Entry](float p_Progress) {
                l_Entry->progress.store(p_Progress,
                                        std::memory_order_relaxed);
                tracking_set_progress(l_Entry->trackedId, p_Progress);
              });
              l_Entry->progress.store(1.0f,
                                      std::memory_order_relaxed);
              l_Entry->status.store(
                  static_cast<int>(JobStatus::Completed),
                  std::memory_order_release);
              tracking_finish(l_Entry->trackedId, true);
            } catch (...) {
              l_Entry->status.store(
                  static_cast<int>(JobStatus::Failed),
                  std::memory_order_release);
              tracking_finish(l_Entry->trackedId, false);
            }
          }
        }

        void initialize(u32 p_NumWorkers)
        {
          g_Stop = false;
          for (u32 i = 0; i < p_NumWorkers; ++i) {
            String i_Name = "BG Worker ";
            i_Name += std::to_string(i + 1).c_str();

            g_Workers.emplace_back([i_Name] {
              Log::set_current_thread_name(i_Name.c_str());
              worker_func();
            });
            set_thread_name(g_Workers[i], i_Name.c_str());
          }
        }

        void cleanup()
        {
          {
            std::unique_lock<std::mutex> l_Lock(g_Mutex);
            g_Stop = true;
          }
          g_Condition.notify_all();
          for (std::thread &i_Worker : g_Workers) {
            i_Worker.join();
          }
          g_Workers.clear();
        }

        JobHandle
        schedule(String p_Label,
                 Function<void(Function<void(float)>)> p_Work,
                 JobPriority p_Priority)
        {
          auto l_Entry = make_shared<BackgroundJobEntry>();
          l_Entry->id =
              g_NextJobId.fetch_add(1, std::memory_order_relaxed);
          l_Entry->trackedId = tracking_begin(
              Tracking::JobType::Background, p_Label, "");
          l_Entry->priority = p_Priority;
          l_Entry->label = std::move(p_Label);
          l_Entry->work = std::move(p_Work);

          {
            std::unique_lock<std::mutex> l_Lock(g_Mutex);
            g_Jobs[l_Entry->id] = l_Entry;
            g_Queue.push({p_Priority, l_Entry->id});
          }
          g_Condition.notify_one();

          return {l_Entry->id};
        }

        JobInfo get_info(JobHandle p_Handle)
        {
          SharedPtr<BackgroundJobEntry> l_Entry;
          {
            std::unique_lock<std::mutex> l_Lock(g_Mutex);
            auto l_It = g_Jobs.find(p_Handle.id);
            if (l_It == g_Jobs.end()) {
              return {JobStatus::Failed, 0.0f, ""};
            }
            l_Entry = l_It->second;
          }
          JobInfo l_Info;
          l_Info.status = static_cast<JobStatus>(
              l_Entry->status.load(std::memory_order_acquire));
          l_Info.progress =
              l_Entry->progress.load(std::memory_order_relaxed);
          l_Info.label = l_Entry->label;
          return l_Info;
        }

        bool is_done(JobHandle p_Handle)
        {
          SharedPtr<BackgroundJobEntry> l_Entry;
          {
            std::unique_lock<std::mutex> l_Lock(g_Mutex);
            auto l_It = g_Jobs.find(p_Handle.id);
            if (l_It == g_Jobs.end()) {
              return true;
            }
            l_Entry = l_It->second;
          }
          int l_Status =
              l_Entry->status.load(std::memory_order_acquire);
          return l_Status == static_cast<int>(JobStatus::Completed) ||
                 l_Status == static_cast<int>(JobStatus::Failed);
        }

        void release(JobHandle p_Handle)
        {
          std::unique_lock<std::mutex> l_Lock(g_Mutex);
          auto l_It = g_Jobs.find(p_Handle.id);
          if (l_It != g_Jobs.end()) {
            tracking_release(l_It->second->trackedId);
            g_Jobs.erase(l_It);
          }
        }

      } // namespace Background

      namespace Tracking {

        bool is_enabled()
        {
#if LOW_JOBMANAGER_TRACKING
          return true;
#else
          return false;
#endif
        }

        bool has_active_jobs()
        {
#if LOW_JOBMANAGER_TRACKING
          std::unique_lock<std::mutex> l_Lock(g_TrackedJobsMutex);
          for (auto &i_Entry : g_TrackedJobs) {
            const Background::JobStatus l_Status =
                static_cast<Background::JobStatus>(
                    i_Entry.second->status.load(
                        std::memory_order_acquire));
            if (l_Status == Background::JobStatus::Pending ||
                l_Status == Background::JobStatus::Running) {
              return true;
            }
          }
#endif
          return false;
        }

        String get_active_job_label()
        {
#if LOW_JOBMANAGER_TRACKING
          std::unique_lock<std::mutex> l_Lock(g_TrackedJobsMutex);
          String l_PendingLabel = "";

          for (auto &i_Entry : g_TrackedJobs) {
            const Background::JobStatus l_Status =
                static_cast<Background::JobStatus>(
                    i_Entry.second->status.load(
                        std::memory_order_acquire));

            if (l_Status == Background::JobStatus::Running) {
              return i_Entry.second->label;
            }

            if (l_Status == Background::JobStatus::Pending &&
                l_PendingLabel.empty()) {
              l_PendingLabel = i_Entry.second->label;
            }
          }

          return l_PendingLabel;
#else
          return "";
#endif
        }

        List<JobSnapshot> get_snapshot()
        {
          List<JobSnapshot> l_Result;
#if LOW_JOBMANAGER_TRACKING
          const auto l_Now = std::chrono::steady_clock::now();

          std::unique_lock<std::mutex> l_Lock(g_TrackedJobsMutex);
          l_Result.reserve(g_TrackedJobs.size());

          for (auto &i_Entry : g_TrackedJobs) {
            SharedPtr<TrackedJob> i_Job = i_Entry.second;
            JobSnapshot i_Snapshot;
            i_Snapshot.id = i_Job->id;
            i_Snapshot.type = i_Job->type;
            i_Snapshot.status = static_cast<Background::JobStatus>(
                i_Job->status.load(std::memory_order_acquire));
            i_Snapshot.progress =
                i_Job->progress.load(std::memory_order_relaxed);
            if (i_Snapshot.status ==
                    Background::JobStatus::Completed ||
                i_Snapshot.status == Background::JobStatus::Failed) {
              i_Snapshot.elapsedMs =
                  i_Job->elapsedMs.load(std::memory_order_relaxed);
            } else {
              i_Snapshot.elapsedMs =
                  static_cast<u64>(std::chrono::duration_cast<
                                       std::chrono::milliseconds>(
                                       l_Now - i_Job->startTime)
                                       .count());
            }
            i_Snapshot.label = i_Job->label;
            i_Snapshot.detail = i_Job->detail;
            l_Result.push_back(i_Snapshot);
          }
#endif
          return l_Result;
        }

        void clear_completed()
        {
#if LOW_JOBMANAGER_TRACKING
          std::unique_lock<std::mutex> l_Lock(g_TrackedJobsMutex);
          List<u64> l_RemoveIds;
          for (auto &i_Entry : g_TrackedJobs) {
            int l_Status = i_Entry.second->status.load(
                std::memory_order_acquire);
            if (l_Status == static_cast<int>(
                                Background::JobStatus::Completed) ||
                l_Status ==
                    static_cast<int>(Background::JobStatus::Failed)) {
              l_RemoveIds.push_back(i_Entry.first);
            }
          }

          for (u64 i_Id : l_RemoveIds) {
            g_TrackedJobs.erase(i_Id);
          }
#endif
        }

      } // namespace Tracking

    } // namespace JobManager
  } // namespace Util
} // namespace Low
