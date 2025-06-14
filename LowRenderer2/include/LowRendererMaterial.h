#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererMaterialType.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API MaterialData
    {
      MaterialType material_type;
      Util::List<uint8_t> data;
      bool dirty;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(MaterialData);
      }
    };

    struct LOW_RENDERER2_API Material : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Material> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Material();
      Material(uint64_t p_Id);
      Material(Material &p_Copy);

    private:
      static Material make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

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
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static Material *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static Material find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

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
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == Material::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Material l_Material = p_Handle.get_id();
        l_Material.destroy();
      }

      MaterialType get_material_type() const;

      bool is_dirty() const;
      void set_dirty(bool p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static Material
      make(Low::Util::Name p_Name,
           Low::Renderer::MaterialType p_MaterialType);
      void *get_data() const;
      void set_property_vector4(Util::Name p_Name,
                                Math::Vector4 &p_Value);
      void set_property_vector3(Util::Name p_Name,
                                Math::Vector3 &p_Value);
      void set_property_vector2(Util::Name p_Name,
                                Math::Vector2 &p_Value);
      void set_property_float(Util::Name p_Name, float p_Value);
      void set_property_u32(Util::Name p_Name, uint32_t p_Value);
      Low::Math::Vector4 &
      get_property_vector4(Util::Name p_Name) const;
      Low::Math::Vector3 &
      get_property_vector3(Util::Name p_Name) const;
      Low::Math::Vector2 &
      get_property_vector2(Util::Name p_Name) const;
      float get_property_float(Util::Name p_Name) const;
      uint32_t get_property_u32(Util::Name p_Name) const;

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      void set_material_type(MaterialType p_Value);
      Util::List<uint8_t> &data() const;

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
