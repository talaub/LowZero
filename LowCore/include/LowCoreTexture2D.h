#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererExposedObjects.h"
#include "LowCoreResource.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_CORE_API Texture2DData
    {
      Util::String path;
      Renderer::Texture2D renderer_texture;
      uint32_t reference_count;
      ResourceState state;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(Texture2DData);
      }
    };

    struct LOW_CORE_API Texture2D : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Texture2D> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Texture2D();
      Texture2D(uint64_t p_Id);
      Texture2D(Texture2D &p_Copy);

    private:
      static Texture2D make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit Texture2D(const Texture2D &p_Copy)
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
      static Texture2D *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static Texture2D find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      Texture2D duplicate(Low::Util::Name p_Name) const;
      static Texture2D duplicate(Texture2D p_Handle,
                                 Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static Texture2D find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == Texture2D::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Texture2D l_Texture2D = p_Handle.get_id();
        l_Texture2D.destroy();
      }

      Util::String &get_path() const;

      Renderer::Texture2D get_renderer_texture() const;

      ResourceState get_state() const;
      void set_state(ResourceState p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static Texture2D make(Util::String &p_Path);
      bool is_loaded();
      void load();
      void unload();
      static void update();

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_path(Util::String &p_Value);
      void set_path(const char *p_Value);
      void set_renderer_texture(Renderer::Texture2D p_Value);
      uint32_t get_reference_count() const;
      void set_reference_count(uint32_t p_Value);
      void _unload();

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Core
} // namespace Low
