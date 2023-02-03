#include "LowRendererCommandBuffer.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t CommandBuffer::TYPE_ID = 5;
      uint8_t *CommandBuffer::ms_Buffer = 0;
      Low::Util::Instances::Slot *CommandBuffer::ms_Slots = 0;
      Low::Util::List<CommandBuffer> CommandBuffer::ms_LivingInstances =
          Low::Util::List<CommandBuffer>();

      CommandBuffer::CommandBuffer() : Low::Util::Handle(0ull)
      {
      }
      CommandBuffer::CommandBuffer(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      CommandBuffer::CommandBuffer(CommandBuffer &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      CommandBuffer CommandBuffer::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        CommandBuffer l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = CommandBuffer::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, CommandBuffer, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void CommandBuffer::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const CommandBuffer *l_Instances = living_instances();
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

      void CommandBuffer::initialize()
      {
        initialize_buffer(&ms_Buffer, CommandBufferData::get_size(),
                          get_capacity(), &ms_Slots);
      }

      void CommandBuffer::cleanup()
      {
        Low::Util::List<CommandBuffer> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);
      }

      bool CommandBuffer::is_alive() const
      {
        return m_Data.m_Type == CommandBuffer::TYPE_ID &&
               check_alive(ms_Slots, CommandBuffer::get_capacity());
      }

      uint32_t CommandBuffer::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity = Low::Util::Config::get_capacity(N(CommandBuffer));
        }
        return l_Capacity;
      }

      Low::Renderer::Backend::CommandBuffer &
      CommandBuffer::get_commandbuffer() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(CommandBuffer, commandbuffer,
                        Low::Renderer::Backend::CommandBuffer);
      }
      void CommandBuffer::set_commandbuffer(
          Low::Renderer::Backend::CommandBuffer &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(CommandBuffer, commandbuffer,
                 Low::Renderer::Backend::CommandBuffer) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_commandbuffer
        // LOW_CODEGEN::END::CUSTOM:SETTER_commandbuffer
      }

      Low::Util::Name CommandBuffer::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(CommandBuffer, name, Low::Util::Name);
      }
      void CommandBuffer::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(CommandBuffer, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      void CommandBuffer::start()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_start
        Backend::commandbuffer_start(get_commandbuffer());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_start
      }

      void CommandBuffer::stop()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_stop
        Backend::commandbuffer_stop(get_commandbuffer());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_stop
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
