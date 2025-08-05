#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowUtilResource.h"
#include "LowRendererImageResourceState.h"
#include "LowRendererTexture.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API ImageResourceData
    {
      Util::String path;
      Util::Resource::ImageMipMaps resource_image;
      Low::Renderer::ImageResourceState state;
      Low::Renderer::Texture texture;
      Low::Util::List<uint8_t> loaded_mips;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(ImageResourceData);
      }
    };

    struct LOW_RENDERER2_API ImageResource : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<ImageResource> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      ImageResource();
      ImageResource(uint64_t p_Id);
      ImageResource(ImageResource &p_Copy);

    private:
      static ImageResource make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit ImageResource(const ImageResource &p_Copy)
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
      static ImageResource *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static ImageResource find_by_index(uint32_t p_Index);
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

      ImageResource duplicate(Low::Util::Name p_Name) const;
      static ImageResource duplicate(ImageResource p_Handle,
                                     Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static ImageResource find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == ImageResource::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        ImageResource l_ImageResource = p_Handle.get_id();
        l_ImageResource.destroy();
      }

      Util::String &get_path() const;

      Util::Resource::ImageMipMaps &get_resource_image() const;

      Low::Renderer::ImageResourceState get_state() const;
      void set_state(Low::Renderer::ImageResourceState p_Value);

      Low::Renderer::Texture &get_texture() const;
      void set_texture(Low::Renderer::Texture &p_Value);

      Low::Util::List<uint8_t> &loaded_mips() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static ImageResource make(Util::String &p_Path);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_path(Util::String &p_Value);
      void set_path(const char *p_Value);
      void set_loaded_mips(Low::Util::List<uint8_t> &p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
