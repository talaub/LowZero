#pragma once

#include "LowUtilApi.h"
#include "LowUtilContainers.h"
#include "LowUtilResource.h"

#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace Low {
  namespace Util {
    namespace Serial {
      struct Node;
    }

    namespace JobManager {

      struct LOW_EXPORT ThreadPool
      {
      public:
        ThreadPool(int p_NumThreads);

        template <class F>
        auto enqueue(F &&p_F) -> Future<decltype(p_F())>
        {
          using ReturnType = decltype(p_F());

          auto l_Task =
              std::make_shared<std::packaged_task<ReturnType()>>(
                  std::forward<F>(p_F));
          Future<ReturnType> l_Result = l_Task->get_future();
          (*l_Task)();
          return l_Result;
        }

        ~ThreadPool();

      private:
        List<std::thread> m_Workers;
        Queue<Function<void()>> m_JobQueue;
        std::mutex m_QueueMutex;
        std::condition_variable m_Condition;
        bool m_Stop;
      };

      LOW_EXPORT void initialize();
      LOW_EXPORT void cleanup();
      LOW_EXPORT ThreadPool &default_pool();

      namespace IO {

        struct LOW_EXPORT MeshLoadResult
        {
          Resource::Mesh mesh;
          Math::AABB aabb;
          Math::Sphere bounding_sphere;
          UnorderedMap<Name, Math::AABB> submesh_aabbs;
          UnorderedMap<Name, Math::Sphere> submesh_bounding_spheres;
        };

        LOW_EXPORT void initialize();
        LOW_EXPORT void cleanup();
        LOW_EXPORT void flush_callbacks();

        LOW_EXPORT void schedule_read_raw(
            String p_Path,
            Function<void(bool, List<uint8_t> &)> p_Callback);

        LOW_EXPORT void schedule_read_texture(
            String p_Path,
            Function<void(bool, Resource::ImageMipMaps &)>
                p_Callback);

        LOW_EXPORT void schedule_read_mesh(
            String p_MeshPath, String p_SidecarPath,
            Function<void(bool, MeshLoadResult &)> p_Callback);

        LOW_EXPORT void schedule_read_yaml(
            String p_Path,
            Function<void(bool, Serial::Node &)> p_Callback);

        LOW_EXPORT void
        schedule_write_yaml(String p_Path, Serial::Node p_Node,
                            Function<void(bool)> p_Callback = nullptr);

      } // namespace IO

      namespace Background {

        enum class JobPriority
        {
          Low = 0,
          Normal = 1,
          High = 2
        };

        enum class JobStatus
        {
          Pending,
          Running,
          Completed,
          Failed
        };

        struct LOW_EXPORT JobHandle
        {
          u64 id = 0;
          bool is_valid() const
          {
            return id != 0;
          }
        };

        struct LOW_EXPORT JobInfo
        {
          JobStatus status;
          float progress;
          String label;
        };

        LOW_EXPORT void initialize(u32 p_NumWorkers = 2);
        LOW_EXPORT void cleanup();

        LOW_EXPORT JobHandle
        schedule(String p_Label,
                 Function<void(Function<void(float)>)> p_Work,
                 JobPriority p_Priority = JobPriority::Normal);

        LOW_EXPORT JobInfo get_info(JobHandle p_Handle);
        LOW_EXPORT bool is_done(JobHandle p_Handle);
        LOW_EXPORT void release(JobHandle p_Handle);

      } // namespace Background

    } // namespace JobManager
  } // namespace Util
} // namespace Low
