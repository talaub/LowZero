#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererMaterialType.h"
#include "LowRendererMaterialState.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererTexture.h"
#include "LowRendererGpuMaterial.h"
#include "LowRendererMaterialResource.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    struct PendingTextureBinding;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API Material : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        MaterialState state;
        MaterialType material_type;
        Low::Renderer::MaterialResource resource;
        Low::Renderer::GpuMaterial gpu;
        Low::Util::Map<Low::Util::Name, Low::Util::Variant>
            properties;
        Low::Util::Set<u64> references;
        Low::Util::UniqueId unique_id;
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

      static Low::Util::List<Material> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Material();
      Material(uint64_t p_Id);
      Material(Material &p_Copy);

    private:
      static Material make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      static Material make(Low::Util::Name p_Name,
                           Low::Util::UniqueId p_UniqueId);

    public:
      explicit Material(const Material &p_Copy)
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
      static Material *living_instances()
      {
        Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        return ms_LivingInstances.data();
      }

      static Material create_handle_by_index(u32 p_Index);

      static Material find_by_index(uint32_t p_Index);
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

      Material duplicate(Low::Util::Name p_Name) const;
      static Material duplicate(Material p_Handle,
                                Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static Material find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        Material l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Material l_Material = p_Handle.get_id();
        l_Material.destroy();
      }

      MaterialState get_state() const;
      void set_state(MaterialState p_Value);

      MaterialType get_material_type() const;

      Low::Renderer::MaterialResource get_resource() const;
      void set_resource(Low::Renderer::MaterialResource p_Value);

      Low::Renderer::GpuMaterial get_gpu() const;
      void set_gpu(Low::Renderer::GpuMaterial p_Value);

      Low::Util::Map<Low::Util::Name, Low::Util::Variant> &
      get_properties() const;

      Low::Util::UniqueId get_unique_id() const;

      void mark_dirty();

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static Material
      make(Low::Util::Name p_Name,
           Low::Renderer::MaterialType p_MaterialType);
      static Material
      make_gpu_ready(Low::Util::Name p_Name,
                     Low::Renderer::MaterialType p_MaterialType);
      void update_gpu();
      void set_property_vector4(Util::Name p_Name,
                                Math::Vector4 &p_Value);
      void set_property_vector3(Util::Name p_Name,
                                Math::Vector3 &p_Value);
      void set_property_vector2(Util::Name p_Name,
                                Math::Vector2 &p_Value);
      void set_property_float(Util::Name p_Name, float p_Value);
      void set_property_u32(Util::Name p_Name, uint32_t p_Value);
      void set_property_texture(Util::Name p_Name,
                                Low::Renderer::Texture p_Value);
      Low::Math::Vector4 &
      get_property_vector4(Util::Name p_Name) const;
      Low::Math::Vector3 &
      get_property_vector3(Util::Name p_Name) const;
      Low::Math::Vector2 &
      get_property_vector2(Util::Name p_Name) const;
      float get_property_float(Util::Name p_Name) const;
      uint32_t get_property_u32(Util::Name p_Name) const;
      Low::Renderer::Texture
      get_property_texture(Util::Name p_Name) const;
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
      void set_material_type(MaterialType p_Value);
      Low::Util::Set<u64> &get_references() const;
      void set_unique_id(Low::Util::UniqueId p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
    public:
      static Util::List<PendingTextureBinding>
          ms_PendingTextureBindings;
      static Util::Mutex ms_PendingTextureBindingsMutex;
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    struct PendingTextureBinding
    {
      Util::Name propertyName;
      Material material;
      Texture texture;
    };
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
