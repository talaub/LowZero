#include "LowRendererUniformScopeInterface.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t UniformScopeInterface::TYPE_ID = 10;
      uint8_t *UniformScopeInterface::ms_Buffer = 0;
      Low::Util::Instances::Slot *UniformScopeInterface::ms_Slots = 0;
      Low::Util::List<UniformScopeInterface>
          UniformScopeInterface::ms_LivingInstances =
              Low::Util::List<UniformScopeInterface>();

      UniformScopeInterface::UniformScopeInterface() : Low::Util::Handle(0ull)
      {
      }
      UniformScopeInterface::UniformScopeInterface(uint64_t p_Id)
          : Low::Util::Handle(p_Id)
      {
      }
      UniformScopeInterface::UniformScopeInterface(
          UniformScopeInterface &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      UniformScopeInterface UniformScopeInterface::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        UniformScopeInterface l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = UniformScopeInterface::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, UniformScopeInterface, name,
                          Low::Util::Name) = Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void UniformScopeInterface::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const UniformScopeInterface *l_Instances = living_instances();
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

      void UniformScopeInterface::initialize()
      {
        initialize_buffer(&ms_Buffer, UniformScopeInterfaceData::get_size(),
                          get_capacity(), &ms_Slots);
      }

      void UniformScopeInterface::cleanup()
      {
        Low::Util::List<UniformScopeInterface> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);
      }

      bool UniformScopeInterface::is_alive() const
      {
        return m_Data.m_Type == UniformScopeInterface::TYPE_ID &&
               check_alive(ms_Slots, UniformScopeInterface::get_capacity());
      }

      uint32_t UniformScopeInterface::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity =
              Low::Util::Config::get_capacity(N(UniformScopeInterface));
        }
        return l_Capacity;
      }

      Backend::UniformScopeInterface &
      UniformScopeInterface::get_interface() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(UniformScopeInterface, interface,
                        Backend::UniformScopeInterface);
      }

      Low::Util::Name UniformScopeInterface::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(UniformScopeInterface, name, Low::Util::Name);
      }
      void UniformScopeInterface::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(UniformScopeInterface, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      UniformScopeInterface
      UniformScopeInterface::make(Util::Name p_Name,
                                  UniformScopeInterfaceCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        UniformScopeInterface l_Interface = UniformScopeInterface::make(p_Name);

        Backend::UniformScopeInterfaceCreateParams l_Params;
        l_Params.context = &(p_Params.context.get_context());
        l_Params.uniformInterfaceCount = p_Params.uniformInterfaces.size();
        l_Params.uniformInterfaces = p_Params.uniformInterfaces.data();

        Backend::uniform_scope_interface_create(l_Interface.get_interface(),
                                                l_Params);

        return l_Interface;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
