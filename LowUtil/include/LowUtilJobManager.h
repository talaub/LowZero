#pragma once

#include "LowUtilApi.h"

#include <thread>
#include <functional>
#include <mutex>

#include "LowUtilContainers.h"

namespace Low {
  namespace Util {
    namespace JobManager {
      struct LOW_EXPORT ThreadPool
      {
      public:
        ThreadPool(int p_NumThreads);

        template <class F> auto enqueue(F &&f) -> Future<decltype(f())>
        {
          using ReturnType = decltype(f());

          auto task = std::make_shared<std::packaged_task<ReturnType()>>(
              std::forward<F>(f));
          std::future<ReturnType> result = task->get_future();

          {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_JobQueue.emplace([task]() { (*task)(); });
          }

          m_Condition.notify_one();

          return result;
        }

        ~ThreadPool();

      private:
        List<std::thread> m_Workers;
        Queue<std::function<void()>> m_JobQueue;
        std::mutex m_QueueMutex;
        std::condition_variable m_Condition;
        bool m_Stop;
      };

      void initialize();
      void cleanup();

      LOW_EXPORT ThreadPool &default_pool();
    } // namespace JobManager
  }   // namespace Util
} // namespace Low
