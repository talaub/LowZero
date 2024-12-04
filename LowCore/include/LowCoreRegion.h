#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowMath.h"
#include "LowCoreScene.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    struct Entity;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_CORE_API RegionData
    {
      bool loaded;
      bool streaming_enabled;
      Math::Vector3 streaming_position;
      float streaming_radius;
      Util::Set<Util::UniqueId> entities;
      Scene scene;
      Low::Util::UniqueId unique_id;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(RegionData);
      }
    };

    struct LOW_CORE_API Region : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Region> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Region();
      Region(uint64_t p_Id);
      Region(Region &p_Copy);

      static Region make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit Region(const Region &p_Copy)
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
      static Region *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static Region find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      Region duplicate(Low::Util::Name p_Name) const;
      static Region duplicate(Region p_Handle,
                              Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static Region find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.get_type() == Region::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Region l_Region = p_Handle.get_id();
        l_Region.destroy();
      }

      bool is_loaded() const;
      void set_loaded(bool p_Value);

      bool is_streaming_enabled() const;
      void set_streaming_enabled(bool p_Value);

      Math::Vector3 &get_streaming_position() const;
      void set_streaming_position(Math::Vector3 &p_Value);

      float get_streaming_radius() const;
      void set_streaming_radius(float p_Value);

      Scene get_scene() const;
      void set_scene(Scene p_Value);

      Low::Util::UniqueId get_unique_id() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      void serialize_entities(Util::Yaml::Node &p_Node);
      void add_entity(Entity p_Entity);
      void remove_entity(Entity p_Entity);
      void load_entities();
      void unload_entities();

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      Util::Set<Util::UniqueId> &get_entities() const;
      void set_unique_id(Low::Util::UniqueId p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Core
} // namespace Low
