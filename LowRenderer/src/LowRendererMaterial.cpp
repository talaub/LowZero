#include "LowRendererMaterial.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Renderer {
    const uint16_t Material::TYPE_ID = 10;
    uint8_t *Material::ms_Buffer = 0;
    Low::Util::Instances::Slot *Material::ms_Slots = 0;
    Low::Util::List<Material> Material::ms_LivingInstances =
        Low::Util::List<Material>();

    Material::Material() : Low::Util::Handle(0ull)
    {
    }
    Material::Material(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Material::Material(Material &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Material Material::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = Low::Util::Instances::create_instance(
          ms_Buffer, ms_Slots, get_capacity());

      MaterialData *l_DataPtr =
          (MaterialData *)&ms_Buffer[l_Index * sizeof(MaterialData)];
      new (l_DataPtr) MaterialData();

      Material l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Material::TYPE_ID;

      ACCESSOR_TYPE_SOA(l_Handle, Material, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      return l_Handle;
    }

    void Material::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const Material *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
      _LOW_ASSERT(l_LivingInstanceFound);
    }

    void Material::initialize()
    {
      initialize_buffer(&ms_Buffer, MaterialData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_Material);
      LOW_PROFILE_ALLOC(type_slots_Material);
    }

    void Material::cleanup()
    {
      Low::Util::List<Material> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Material);
      LOW_PROFILE_FREE(type_slots_Material);
    }

    bool Material::is_alive() const
    {
      return m_Data.m_Type == Material::TYPE_ID &&
             check_alive(ms_Slots, Material::get_capacity());
    }

    uint32_t Material::get_capacity()
    {
      static uint32_t l_Capacity = 0u;
      if (l_Capacity == 0u) {
        l_Capacity =
            Low::Util::Config::get_capacity(N(LowRenderer), N(Material));
      }
      return l_Capacity;
    }

    MaterialType Material::get_material_type() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Material, material_type, MaterialType);
    }
    void Material::set_material_type(MaterialType p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(Material, material_type, MaterialType) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material_type
      // LOW_CODEGEN::END::CUSTOM:SETTER_material_type
    }

    Low::Util::Name Material::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Material, name, Low::Util::Name);
    }
    void Material::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(Material, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

  } // namespace Renderer
} // namespace Low
