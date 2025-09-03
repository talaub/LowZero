#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    struct FontResourceConfig
    {
      Util::Name name;
      u64 fontId;
      u64 assetHash;
      Util::String sourceFile;
      Util::String sidecarPath;
      Util::String path;
      Util::String fontPath;
    };
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API FontResourceData
    {
      Util::String path;
      Util::String font_path;
      Util::String sidecar_path;
      Util::String source_file;
      uint64_t font_id;
      uint64_t asset_hash;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(FontResourceData);
      }
    };

    struct LOW_RENDERER2_API FontResource : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<FontResource> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      FontResource();
      FontResource(uint64_t p_Id);
      FontResource(FontResource &p_Copy);

    private:
      static FontResource make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit FontResource(const FontResource &p_Copy)
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
      static FontResource *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static FontResource create_handle_by_index(u32 p_Index);

      static FontResource find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      u64 observe(Low::Util::Name p_Observable,
                  Low::Util::Handle p_Observer) const;
      u64 observe(Low::Util::Name p_Observable,
                  Low::Util::Function<void(Low::Util::Handle,
                                           Low::Util::Name)>
                      p_Observer) const;
      void notify(Low::Util::Handle p_Observed,
                  Low::Util::Name p_Observable);
      void broadcast_observable(Low::Util::Name p_Observable) const;

      static void _notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable);

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      FontResource duplicate(Low::Util::Name p_Name) const;
      static FontResource duplicate(FontResource p_Handle,
                                    Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static FontResource find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == FontResource::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        FontResource l_FontResource = p_Handle.get_id();
        l_FontResource.destroy();
      }

      Util::String &get_path() const;

      Util::String &get_font_path() const;

      Util::String &get_sidecar_path() const;

      Util::String &get_source_file() const;

      uint64_t &get_font_id() const;

      uint64_t &get_asset_hash() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static FontResource make(Util::String &p_Path);
      static FontResource
      make_from_config(FontResourceConfig &p_Config);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_path(Util::String &p_Value);
      void set_path(const char *p_Value);
      void set_font_path(Util::String &p_Value);
      void set_font_path(const char *p_Value);
      void set_sidecar_path(Util::String &p_Value);
      void set_sidecar_path(const char *p_Value);
      void set_source_file(Util::String &p_Value);
      void set_source_file(const char *p_Value);
      void set_font_id(uint64_t &p_Value);
      void set_asset_hash(uint64_t &p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
