#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererTexture.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    enum class TextureExportState
    {
      SCHEDULED,
      COPIED,
      DONE
    };

    struct TextureExport;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API TextureExportData
    {
      Low::Util::String path;
      Low::Renderer::Texture texture;
      Low::Renderer::TextureExportState state;
      Low::Util::Function<bool(Low::Renderer::TextureExport)>
          finish_callback;
      uint64_t data_handle;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(TextureExportData);
      }
    };

    struct LOW_RENDERER2_API TextureExport : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<TextureExport> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      TextureExport();
      TextureExport(uint64_t p_Id);
      TextureExport(TextureExport &p_Copy);

      static TextureExport make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit TextureExport(const TextureExport &p_Copy)
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
      static TextureExport *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static TextureExport find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      u64 observe(Low::Util::Name p_Observable,
                  Low::Util::Handle p_Observer) const;
      void notify(Low::Util::Handle p_Observed,
                  Low::Util::Name p_Observable);
      void broadcast_observable(Low::Util::Name p_Observable) const;

      static void _notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable);

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      TextureExport duplicate(Low::Util::Name p_Name) const;
      static TextureExport duplicate(TextureExport p_Handle,
                                     Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static TextureExport find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == TextureExport::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        TextureExport l_TextureExport = p_Handle.get_id();
        l_TextureExport.destroy();
      }

      Low::Util::String &get_path() const;
      void set_path(Low::Util::String &p_Value);
      void set_path(const char *p_Value);

      Low::Renderer::Texture get_texture() const;
      void set_texture(Low::Renderer::Texture p_Value);

      Low::Renderer::TextureExportState get_state() const;
      void set_state(Low::Renderer::TextureExportState p_Value);

      Low::Util::Function<bool(Low::Renderer::TextureExport)> &
      get_finish_callback() const;
      void set_finish_callback(
          Low::Util::Function<bool(Low::Renderer::TextureExport)>
              &p_Value);

      uint64_t get_data_handle() const;
      void set_data_handle(uint64_t p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
