#include "LowRendererMaterialType.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Renderer {
    const uint16_t MaterialType::TYPE_ID = 9;
    uint8_t *MaterialType::ms_Buffer = 0;
    Low::Util::Instances::Slot *MaterialType::ms_Slots = 0;
    Low::Util::List<MaterialType> MaterialType::ms_LivingInstances =
        Low::Util::List<MaterialType>();

    MaterialType::MaterialType() : Low::Util::Handle(0ull)
    {
    }
    MaterialType::MaterialType(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    MaterialType::MaterialType(MaterialType &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    MaterialType MaterialType::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = Low::Util::Instances::create_instance(
          ms_Buffer, ms_Slots, get_capacity());

      MaterialTypeData *l_DataPtr =
          (MaterialTypeData *)&ms_Buffer[l_Index * sizeof(MaterialTypeData)];
      new (l_DataPtr) MaterialTypeData();

      MaterialType l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = MaterialType::TYPE_ID;

      ACCESSOR_TYPE_SOA(l_Handle, MaterialType, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      return l_Handle;
    }

    void MaterialType::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const MaterialType *l_Instances = living_instances();
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

    void MaterialType::initialize()
    {
      initialize_buffer(&ms_Buffer, MaterialTypeData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_MaterialType);
      LOW_PROFILE_ALLOC(type_slots_MaterialType);
    }

    void MaterialType::cleanup()
    {
      Low::Util::List<MaterialType> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_MaterialType);
      LOW_PROFILE_FREE(type_slots_MaterialType);
    }

    bool MaterialType::is_alive() const
    {
      return m_Data.m_Type == MaterialType::TYPE_ID &&
             check_alive(ms_Slots, MaterialType::get_capacity());
    }

    uint32_t MaterialType::get_capacity()
    {
      static uint32_t l_Capacity = 0u;
      if (l_Capacity == 0u) {
        l_Capacity =
            Low::Util::Config::get_capacity(N(LowRenderer), N(MaterialType));
      }
      return l_Capacity;
    }

    GraphicsPipelineConfig &MaterialType::get_gbuffer_pipeline() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(MaterialType, gbuffer_pipeline, GraphicsPipelineConfig);
    }
    void MaterialType::set_gbuffer_pipeline(GraphicsPipelineConfig &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(MaterialType, gbuffer_pipeline, GraphicsPipelineConfig) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gbuffer_pipeline
      // LOW_CODEGEN::END::CUSTOM:SETTER_gbuffer_pipeline
    }

    Util::List<MaterialTypeProperty> &MaterialType::get_properties() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(MaterialType, properties,
                      Util::List<MaterialTypeProperty>);
    }
    void MaterialType::set_properties(Util::List<MaterialTypeProperty> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(MaterialType, properties, Util::List<MaterialTypeProperty>) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_properties
      // LOW_CODEGEN::END::CUSTOM:SETTER_properties
    }

    Low::Util::Name MaterialType::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(MaterialType, name, Low::Util::Name);
    }
    void MaterialType::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(MaterialType, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

  } // namespace Renderer
} // namespace Low
