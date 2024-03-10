#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_CORE_API SceneData
    {
      Util::Set<Util::UniqueId> regions;
      bool loaded;
      Low::Util::UniqueId unique_id;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(SceneData);
      }
    };

    struct LOW_CORE_API Scene : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Scene> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Scene();
      Scene(uint64_t p_Id);
      Scene(Scene &p_Copy);

      static Scene make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit Scene(const Scene &p_Copy)
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
      static Scene *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static Scene find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      Scene duplicate(Low::Util::Name p_Name) const;
      static Scene duplicate(Scene p_Handle, Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static Scene find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.get_type() == Scene::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Scene l_Scene = p_Handle.get_id();
        l_Scene.destroy();
      }

      Util::Set<Util::UniqueId> &get_regions() const;

      bool is_loaded() const;

      Low::Util::UniqueId get_unique_id() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      void load();
      void unload();
      static Scene get_loaded_scene();

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_loaded(bool p_Value);
      void set_unique_id(Low::Util::UniqueId p_Value);
      void _load();
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Core
} // namespace Low
