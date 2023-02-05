#include "LowRendererPipelineInterface.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t PipelineInterface::TYPE_ID = 8;
      uint8_t *PipelineInterface::ms_Buffer = 0;
      Low::Util::Instances::Slot *PipelineInterface::ms_Slots = 0;
      Low::Util::List<PipelineInterface> PipelineInterface::ms_LivingInstances =
          Low::Util::List<PipelineInterface>();

      PipelineInterface::PipelineInterface() : Low::Util::Handle(0ull)
      {
      }
      PipelineInterface::PipelineInterface(uint64_t p_Id)
          : Low::Util::Handle(p_Id)
      {
      }
      PipelineInterface::PipelineInterface(PipelineInterface &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      PipelineInterface PipelineInterface::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        PipelineInterface l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = PipelineInterface::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, PipelineInterface, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void PipelineInterface::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        Backend::pipeline_interface_cleanup(get_interface());
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const PipelineInterface *l_Instances = living_instances();
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

      void PipelineInterface::initialize()
      {
        initialize_buffer(&ms_Buffer, PipelineInterfaceData::get_size(),
                          get_capacity(), &ms_Slots);
      }

      void PipelineInterface::cleanup()
      {
        Low::Util::List<PipelineInterface> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);
      }

      bool PipelineInterface::is_alive() const
      {
        return m_Data.m_Type == PipelineInterface::TYPE_ID &&
               check_alive(ms_Slots, PipelineInterface::get_capacity());
      }

      uint32_t PipelineInterface::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity = Low::Util::Config::get_capacity(N(PipelineInterface));
        }
        return l_Capacity;
      }

      Low::Renderer::Backend::PipelineInterface &
      PipelineInterface::get_interface() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(PipelineInterface, interface,
                        Low::Renderer::Backend::PipelineInterface);
      }

      Low::Util::Name PipelineInterface::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(PipelineInterface, name, Low::Util::Name);
      }
      void PipelineInterface::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(PipelineInterface, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      PipelineInterface
      PipelineInterface::make(Util::Name p_Name,
                              PipelineInterfaceCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        PipelineInterface l_Interface = PipelineInterface::make(p_Name);

        Backend::PipelineInterfaceCreateParams l_Params;
        l_Params.context = &(p_Params.context.get_context());

        Backend::pipeline_interface_create(l_Interface.get_interface(),
                                           l_Params);

        return l_Interface;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
