#include "LowUtilJobManager.h"

namespace Low {
  namespace Util {
    namespace JobManager {
      ThreadPool *g_DefaultThreadPool;

      ThreadPool::ThreadPool(int p_NumThreads) : m_Stop(false)
      {
        for (int i = 0; i < p_NumThreads; ++i) {
          m_Workers.emplace_back([this] {
            while (true) {
              std::function<void()> task;

              {
                std::unique_lock<std::mutex> lock(m_QueueMutex);
                m_Condition.wait(
                    lock, [this] { return m_Stop || !m_JobQueue.empty(); });

                if (m_Stop && m_JobQueue.empty())
                  return;

                task = std::move(m_JobQueue.front());
                m_JobQueue.pop();
              }

              task();
            }
          });
        }
      }

      ThreadPool::~ThreadPool()
      {
        {
          std::unique_lock<std::mutex> lock(m_QueueMutex);
          m_Stop = true;
        }

        m_Condition.notify_all();

        for (std::thread &worker : m_Workers)
          worker.join();
      }

      void initialize()
      {
        g_DefaultThreadPool = new ThreadPool(4);
      }

      void cleanup()
      {
        delete g_DefaultThreadPool;
      }

      ThreadPool &default_pool()
      {
        return *g_DefaultThreadPool;
      }
    } // namespace JobManager
  }   // namespace Util
} // namespace Low
