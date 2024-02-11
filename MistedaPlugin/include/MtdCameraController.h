#pragma once

#include "MtdPluginApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowCoreEntity.h"

#include "LowMath.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Mtd {
  namespace Component {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct MISTEDA_API CameraControllerData
    {
      float distance;
      Low::Core::Entity entity;
      Low::Util::UniqueId unique_id;

      static size_t get_size()
      {
        return sizeof(CameraControllerData);
      }
    };

    struct MISTEDA_API CameraController : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<CameraController> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      CameraController();
      CameraController(uint64_t p_Id);
      CameraController(CameraController &p_Copy);

      static CameraController make(Low::Core::Entity p_Entity);
      static Low::Util::Handle _make(Low::Util::Handle p_Entity);
      explicit CameraController(const CameraController &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static CameraController *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static CameraController find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.get_type() == CameraController::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        CameraController l_CameraController = p_Handle.get_id();
        l_CameraController.destroy();
      }

      float get_distance() const;
      void set_distance(float p_Value);

      Low::Core::Entity get_entity() const;
      void set_entity(Low::Core::Entity p_Value);

      Low::Util::UniqueId get_unique_id() const;

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_unique_id(Low::Util::UniqueId p_Value);
    };
  } // namespace Component
} // namespace Mtd
