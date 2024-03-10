#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowCoreTexture2D.h"
#include "LowRendererExposedObjects.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_CORE_API MaterialData
    {
      Renderer::MaterialType material_type;
      Renderer::Material renderer_material;
      Util::Map<Util::Name, Util::Variant> properties;
      uint32_t reference_count;
      Low::Util::UniqueId unique_id;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(MaterialData);
      }
    };

    struct LOW_CORE_API Material : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Material> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Material();
      Material(uint64_t p_Id);
      Material(Material &p_Copy);

      static Material make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit Material(const Material &p_Copy)
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
      static Material *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static Material find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      Material duplicate(Low::Util::Name p_Name) const;
      static Material duplicate(Material p_Handle,
                                Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static Material find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.get_type() == Material::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Material l_Material = p_Handle.get_id();
        l_Material.destroy();
      }

      Renderer::MaterialType get_material_type() const;
      void set_material_type(Renderer::MaterialType p_Value);

      Renderer::Material get_renderer_material() const;

      Util::Map<Util::Name, Util::Variant> &get_properties() const;

      Low::Util::UniqueId get_unique_id() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      void set_property(Util::Name p_Name, Util::Variant &p_Value);
      Util::Variant &get_property(Util::Name p_Name);
      bool is_loaded();
      void load();
      void unload();

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_renderer_material(Renderer::Material p_Value);
      uint32_t get_reference_count() const;
      void set_reference_count(uint32_t p_Value);
      void set_unique_id(Low::Util::UniqueId p_Value);
      void _unload();
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Core
} // namespace Low
