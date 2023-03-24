#include "LowRendererRenderFlow.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowRendererComputeStep.h"
#include "LowRendererGraphicsStep.h"

namespace Low {
  namespace Renderer {
    const uint16_t RenderFlow::TYPE_ID = 1;
    uint8_t *RenderFlow::ms_Buffer = 0;
    Low::Util::Instances::Slot *RenderFlow::ms_Slots = 0;
    Low::Util::List<RenderFlow> RenderFlow::ms_LivingInstances =
        Low::Util::List<RenderFlow>();

    RenderFlow::RenderFlow() : Low::Util::Handle(0ull)
    {
    }
    RenderFlow::RenderFlow(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    RenderFlow::RenderFlow(RenderFlow &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    RenderFlow RenderFlow::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = Low::Util::Instances::create_instance(
          ms_Buffer, ms_Slots, get_capacity());

      RenderFlowData *l_DataPtr =
          (RenderFlowData *)&ms_Buffer[l_Index * sizeof(RenderFlowData)];
      new (l_DataPtr) RenderFlowData();

      RenderFlow l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = RenderFlow::TYPE_ID;

      ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      return l_Handle;
    }

    void RenderFlow::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const RenderFlow *l_Instances = living_instances();
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

    void RenderFlow::initialize()
    {
      initialize_buffer(&ms_Buffer, RenderFlowData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_RenderFlow);
      LOW_PROFILE_ALLOC(type_slots_RenderFlow);
    }

    void RenderFlow::cleanup()
    {
      Low::Util::List<RenderFlow> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_RenderFlow);
      LOW_PROFILE_FREE(type_slots_RenderFlow);
    }

    bool RenderFlow::is_alive() const
    {
      return m_Data.m_Type == RenderFlow::TYPE_ID &&
             check_alive(ms_Slots, RenderFlow::get_capacity());
    }

    uint32_t RenderFlow::get_capacity()
    {
      static uint32_t l_Capacity = 0u;
      if (l_Capacity == 0u) {
        l_Capacity =
            Low::Util::Config::get_capacity(N(LowRenderer), N(RenderFlow));
      }
      return l_Capacity;
    }

    Math::UVector2 &RenderFlow::get_dimensions() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, dimensions, Math::UVector2);
    }

    Util::List<Util::Handle> &RenderFlow::get_steps() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, steps, Util::List<Util::Handle>);
    }
    void RenderFlow::set_steps(Util::List<Util::Handle> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderFlow, steps, Util::List<Util::Handle>) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_steps
      // LOW_CODEGEN::END::CUSTOM:SETTER_steps
    }

    ResourceRegistry &RenderFlow::get_resources() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, resources, ResourceRegistry);
    }

    Low::Util::Name RenderFlow::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, name, Low::Util::Name);
    }
    void RenderFlow::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderFlow, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    RenderFlow RenderFlow::make(Util::Name p_Name, Interface::Context p_Context,
                                Util::Yaml::Node &p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      RenderFlow l_RenderFlow = RenderFlow::make(p_Name);
      l_RenderFlow.get_dimensions() = p_Context.get_dimensions();

      if (p_Config["resources"]) {
        Util::List<ResourceConfig> l_ResourceConfigs;
        parse_resource_configs(p_Config["resources"], l_ResourceConfigs);
        l_RenderFlow.get_resources().initialize(l_ResourceConfigs, p_Context,
                                                l_RenderFlow);
      }

      for (auto it = p_Config["steps"].begin(); it != p_Config["steps"].end();
           ++it) {
        Util::Yaml::Node i_StepEntry = *it;
        Util::Name i_StepName = LOW_YAML_AS_NAME(i_StepEntry["name"]);

        bool i_Found = false;
        for (ComputeStepConfig i_Config :
             ComputeStepConfig::ms_LivingInstances) {
          if (i_Config.get_name() == i_StepName) {
            i_Found = true;
            ComputeStep i_Step =
                ComputeStep::make(i_Config.get_name(), p_Context, i_Config);
            i_Step.prepare(l_RenderFlow);

            l_RenderFlow.get_steps().push_back(i_Step);
            break;
          }
        }

        if (!i_Found) {
          for (GraphicsStepConfig i_Config :
               GraphicsStepConfig ::ms_LivingInstances) {
            if (i_Config.get_name() == i_StepName) {
              i_Found = true;
              GraphicsStep i_Step =
                  GraphicsStep::make(i_Config.get_name(), p_Context, i_Config);
              i_Step.prepare(l_RenderFlow);

              l_RenderFlow.get_steps().push_back(i_Step);
              break;
            }
          }
        }

        LOW_ASSERT(i_Found, "Could not find renderstep");
      }

      return l_RenderFlow;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    void RenderFlow::execute()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_execute
      for (Util::Handle i_Step : get_steps()) {
        if (i_Step.get_type() == ComputeStep::TYPE_ID) {
          ComputeStep i_ComputeStep = i_Step.get_id();
          i_ComputeStep.execute(*this);
        } else if (i_Step.get_type() == GraphicsStep::TYPE_ID) {
          GraphicsStep i_GraphicsStep = i_Step.get_id();
          i_GraphicsStep.execute(*this);
        } else {
          LOW_ASSERT(false, "Unknown step type");
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_execute
    }

  } // namespace Renderer
} // namespace Low
