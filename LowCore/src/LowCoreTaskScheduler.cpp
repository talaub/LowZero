#include "LowCoreTaskScheduler.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilContainers.h"

#include "LowCoreMeshResource.h"

namespace Low {
  namespace Core {
    namespace TaskScheduler {
      template <typename T> struct SchedulingQueue
      {
        void schedule(T p_Element)
        {
          if (m_Set.find(p_Element) == m_Set.end()) {
            m_Set.insert(p_Element);
            m_Queue.push(p_Element);
          }
        }

        uint32_t size() const
        {
          return m_Queue.size();
        }

        T pop()
        {
          T l_Element = m_Queue.front();
          m_Queue.pop();
          m_Set.erase(l_Element);

          return l_Element;
        }

        bool empty() const
        {
          return m_Queue.empty();
        }

      private:
        Util::Set<T> m_Set;
        Util::Queue<T> m_Queue;
      };

      SchedulingQueue<MeshResource> g_MeshResourceLoadQueue;

      void schedule_mesh_resource_load(MeshResource p_MeshResource)
      {
        g_MeshResourceLoadQueue.schedule(p_MeshResource);
      }

      static void do_tick()
      {
        if (!g_MeshResourceLoadQueue.empty()) {
          g_MeshResourceLoadQueue.pop().load();
          return;
        }
      }

      void tick()
      {
        do_tick();
      }

    } // namespace TaskScheduler
  }   // namespace Core
} // namespace Low
