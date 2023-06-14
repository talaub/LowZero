#include "LowCoreMonoUtils.h"

#include "LowUtilHandle.h"
#include "LowUtilLogger.h"

#include "LowCoreEntity.h"

namespace Low {
  namespace Core {
    namespace Mono {
      uint64_t entity_get_component(uint64_t p_HandleId, uint16_t p_TypeId)
      {
        Entity l_Entity = p_HandleId;

        Util::Handle l_Handle = l_Entity.get_component(p_TypeId);
        return l_Handle.get_id();
      }
    } // namespace Mono
  }   // namespace Core
} // namespace Low
