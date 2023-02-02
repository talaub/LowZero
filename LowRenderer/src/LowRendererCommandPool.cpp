#include "LowRendererCommandPool.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t CommandPool::TYPE_ID = 6;
      uint8_t *CommandPool::ms_Buffer = 0;
      Low::Util::Instances::Slot *CommandPool::ms_Slots = 0;
      Low::Util::List<CommandPool> CommandPool::ms_LivingInstances =
          Low::Util::List<CommandPool>();

      CommandPool::CommandPool() : Low::Util::Handle(0ull)
      {
      }
      CommandPool::CommandPool(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      CommandPool::CommandPool(CommandPool &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      CommandPool CommandPool::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        CommandPool l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = CommandPool::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, CommandPool, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        return l_Handle;
      }

      void CommandPool::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const CommandPool *l_Instances = living_instances();
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

      void CommandPool::cleanup()
      {
        CommandPool *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);
      }

      bool CommandPool::is_alive() const
      {
        return m_Data.m_Type == CommandPool::TYPE_ID &&
               check_alive(ms_Slots, CommandPool::get_capacity());
      }

      uint32_t CommandPool::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity = Low::Util::Config::get_capacity(N(CommandPool));
        }
        return l_Capacity;
      }

      Low::Renderer::Backend::CommandPool &CommandPool::get_commandpool() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(CommandPool, commandpool,
                        Low::Renderer::Backend::CommandPool);
      }

      Low::Util::Name CommandPool::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(CommandPool, name, Low::Util::Name);
      }
      void CommandPool::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(CommandPool, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
