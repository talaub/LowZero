#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "imgui.h"
#include "LowRendererGpuTexture.h"
#include "LowRendererTextureStaging.h"
#include "LowRendererTextureResource.h"
#include "LowRendererTextureState.h"
#include "LowRendererEditorImage.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API TextureData
    {
      GpuTexture gpu;
      TextureResource resource;
      TextureStaging staging;
      Low::Renderer::TextureState state;
      Low::Util::Set<u64> references;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(TextureData);
      }
    };

    struct LOW_RENDERER2_API Texture : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Texture> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Texture();
      Texture(uint64_t p_Id);
      Texture(Texture &p_Copy);

      static Texture make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit Texture(const Texture &p_Copy)
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
      static Texture *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static Texture create_handle_by_index(u32 p_Index);

      static Texture find_by_index(uint32_t p_Index);
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

      void reference(const u64 p_Id);
      void dereference(const u64 p_Id);
      u32 references() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      Texture duplicate(Low::Util::Name p_Name) const;
      static Texture duplicate(Texture p_Handle,
                               Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static Texture find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == Texture::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Texture l_Texture = p_Handle.get_id();
        l_Texture.destroy();
      }

      GpuTexture get_gpu() const;
      void set_gpu(GpuTexture p_Value);

      TextureResource get_resource() const;
      void set_resource(TextureResource p_Value);

      TextureStaging get_staging() const;
      void set_staging(TextureStaging p_Value);

      Low::Renderer::TextureState get_state() const;
      void set_state(Low::Renderer::TextureState p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static Low::Renderer::Texture
      make_gpu_ready(Low::Util::Name p_Name);
      EditorImage get_editor_image();

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      Low::Util::Set<u64> &get_references() const;

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
