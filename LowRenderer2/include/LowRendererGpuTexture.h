#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include <imgui.h>
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API GpuTextureData
    {
      uint64_t data_handle;
      ImTextureID imgui_texture_id;
      uint8_t full_mip_count;
      Low::Util::List<uint8_t> loaded_mips;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(GpuTextureData);
      }
    };

    struct LOW_RENDERER2_API GpuTexture : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<GpuTexture> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      GpuTexture();
      GpuTexture(uint64_t p_Id);
      GpuTexture(GpuTexture &p_Copy);

      static GpuTexture make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit GpuTexture(const GpuTexture &p_Copy)
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
      static GpuTexture *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static GpuTexture find_by_index(uint32_t p_Index);
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

      GpuTexture duplicate(Low::Util::Name p_Name) const;
      static GpuTexture duplicate(GpuTexture p_Handle,
                                  Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static GpuTexture find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == GpuTexture::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        GpuTexture l_GpuTexture = p_Handle.get_id();
        l_GpuTexture.destroy();
      }

      uint64_t get_data_handle() const;
      void set_data_handle(uint64_t p_Value);

      ImTextureID get_imgui_texture_id() const;
      void set_imgui_texture_id(ImTextureID p_Value);

      uint8_t get_full_mip_count() const;
      void set_full_mip_count(uint8_t p_Value);

      Low::Util::List<uint8_t> &loaded_mips() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      void set_loaded_mips(Low::Util::List<uint8_t> &p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
    public:
      static Low::Util::Set<Low::Renderer::GpuTexture> ms_Dirty;
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
