#include "LowUtilJobManager.h"
#include "LowUtilSerialization.h"
#include "LowMath.h"

#include <fstream>
#include <string>

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
          m_Workers.emplace_back([this] {
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

          Util::String i_Name = "Worker ";
          i_Name += std::to_string(i + 1).c_str();
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
          g_IoThread = std::thread(io_worker_func);
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
          io_enqueue([l_Path = std::move(p_Path),
                      l_Callback = std::move(p_Callback)]() mutable {
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
            io_post_completion(
                [l_Success, l_Data = std::move(l_Data),
                 l_Callback = std::move(l_Callback)]() mutable {
                  l_Callback(l_Success, l_Data);
                });
          });
        }

        void schedule_read_texture(
            String p_Path,
            Function<void(bool, Resource::ImageMipMaps &)> p_Callback)
        {
          io_enqueue([l_Path = std::move(p_Path),
                      l_Callback = std::move(p_Callback)]() mutable {
            bool l_Success = false;
            Resource::ImageMipMaps l_MipMaps;
            try {
              Resource::load_image_mipmaps(l_Path, l_MipMaps);
              l_Success = true;
            } catch (...) {
            }
            io_post_completion(
                [l_Success, l_MipMaps = std::move(l_MipMaps),
                 l_Callback = std::move(l_Callback)]() mutable {
                  l_Callback(l_Success, l_MipMaps);
                });
          });
        }

        void schedule_read_mesh(
            String p_MeshPath, String p_SidecarPath,
            Function<void(bool, MeshLoadResult &)> p_Callback)
        {
          io_enqueue([l_MeshPath = std::move(p_MeshPath),
                      l_SidecarPath = std::move(p_SidecarPath),
                      l_Callback = std::move(p_Callback)]() mutable {
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
            io_post_completion(
                [l_Success, l_Result = std::move(l_Result),
                 l_Callback = std::move(l_Callback)]() mutable {
                  l_Callback(l_Success, l_Result);
                });
          });
        }

        void schedule_read_yaml(
            String p_Path,
            Function<void(bool, Serial::Node &)> p_Callback)
        {
          io_enqueue([l_Path = std::move(p_Path),
                      l_Callback = std::move(p_Callback)]() mutable {
            bool l_Success = false;
            Serial::Node l_Node;
            try {
              l_Node = Serial::load_yaml_file(l_Path.c_str());
              l_Success = true;
            } catch (...) {
            }
            io_post_completion(
                [l_Success, l_Node = std::move(l_Node),
                 l_Callback = std::move(l_Callback)]() mutable {
                  l_Callback(l_Success, l_Node);
                });
          });
        }

        void schedule_write_yaml(String p_Path, Serial::Node p_Node,
                                  Function<void(bool)> p_Callback)
        {
          io_enqueue([l_Path = std::move(p_Path),
                      l_Node = std::move(p_Node),
                      l_Callback = std::move(p_Callback)]() mutable {
            bool l_Success = false;
            try {
              Serial::write_yaml_file(l_Path.c_str(), l_Node);
              l_Success = true;
            } catch (...) {
            }
            if (l_Callback) {
              io_post_completion(
                  [l_Success,
                   l_Callback = std::move(l_Callback)]() mutable {
                    l_Callback(l_Success);
                  });
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
            }

            try {
              l_Entry->work([&l_Entry](float p_Progress) {
                l_Entry->progress.store(p_Progress,
                                        std::memory_order_relaxed);
              });
              l_Entry->progress.store(1.0f,
                                      std::memory_order_relaxed);
              l_Entry->status.store(
                  static_cast<int>(JobStatus::Completed),
                  std::memory_order_release);
            } catch (...) {
              l_Entry->status.store(
                  static_cast<int>(JobStatus::Failed),
                  std::memory_order_release);
            }
          }
        }

        void initialize(u32 p_NumWorkers)
        {
          g_Stop = false;
          for (u32 i = 0; i < p_NumWorkers; ++i) {
            g_Workers.emplace_back(worker_func);
            String i_Name = "BG Worker ";
            i_Name += std::to_string(i + 1).c_str();
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
          g_Jobs.erase(p_Handle.id);
        }

      } // namespace Background

    } // namespace JobManager
  } // namespace Util
} // namespace Low
