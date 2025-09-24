#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    struct TextureResourceConfig
    {
      Util::Name name;
      u64 textureId;
      u64 assetHash;
      Util::String sourceFile;
      Util::String sidecarPath;
      Util::String path;
      Util::String texturePath;
    };
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API TextureResource
        : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        Util::String path;
        Util::String texture_path;
        Util::String sidecar_path;
        Util::String source_file;
        uint64_t texture_id;
        uint64_t asset_hash;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(Data);
        }
      };

    public:
      static Low::Util::SharedMutex ms_LivingMutex;
      static Low::Util::UniqueLock<Low::Util::SharedMutex>
          ms_PagesLock;
      static Low::Util::SharedMutex ms_PagesMutex;
      static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

      static Low::Util::List<TextureResource> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      TextureResource();
      TextureResource(uint64_t p_Id);
      TextureResource(TextureResource &p_Copy);

    private:
      static TextureResource make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit TextureResource(const TextureResource &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      static uint32_t living_count()
      {
        Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static TextureResource *living_instances()
      {
        Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        return ms_LivingInstances.data();
      }

      static TextureResource create_handle_by_index(u32 p_Index);

      static TextureResource find_by_index(uint32_t p_Index);
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

      TextureResource duplicate(Low::Util::Name p_Name) const;
      static TextureResource duplicate(TextureResource p_Handle,
                                       Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static TextureResource find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        TextureResource l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        TextureResource l_TextureResource = p_Handle.get_id();
        l_TextureResource.destroy();
      }

      Util::String &get_path() const;

      Util::String &get_texture_path() const;

      Util::String &get_sidecar_path() const;

      Util::String &get_source_file() const;

      uint64_t get_texture_id() const;

      uint64_t get_asset_hash() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static TextureResource make(Util::String &p_Path);
      static TextureResource
      make_from_config(TextureResourceConfig &p_Config);
      static TextureResource find_by_path(Util::String &p_Path);
      static bool get_page_for_index(const u32 p_Index,
                                     u32 &p_PageIndex,
                                     u32 &p_SlotIndex);

    private:
      static u32 ms_Capacity;
      static u32 ms_PageSize;
      static u32 create_instance(
          u32 &p_PageIndex, u32 &p_SlotIndex,
          Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock);
      static u32 create_page();
      void set_path(Util::String &p_Value);
      void set_path(const char *p_Value);
      void set_texture_path(Util::String &p_Value);
      void set_texture_path(const char *p_Value);
      void set_sidecar_path(Util::String &p_Value);
      void set_sidecar_path(const char *p_Value);
      void set_source_file(Util::String &p_Value);
      void set_source_file(const char *p_Value);
      void set_texture_id(uint64_t p_Value);
      void set_asset_hash(uint64_t p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
