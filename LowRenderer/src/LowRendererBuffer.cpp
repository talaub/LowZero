#include "LowRendererBuffer.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t Buffer::TYPE_ID = 14;
      uint8_t *Buffer::ms_Buffer = 0;
      Low::Util::Instances::Slot *Buffer::ms_Slots = 0;
      Low::Util::List<Buffer> Buffer::ms_LivingInstances =
          Low::Util::List<Buffer>();

      Buffer::Buffer() : Low::Util::Handle(0ull)
      {
      }
      Buffer::Buffer(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      Buffer::Buffer(Buffer &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Buffer Buffer::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        Buffer l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Buffer::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, Buffer, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void Buffer::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const Buffer *l_Instances = living_instances();
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

      void Buffer::initialize()
      {
        initialize_buffer(&ms_Buffer, BufferData::get_size(), get_capacity(),
                          &ms_Slots);
      }

      void Buffer::cleanup()
      {
        Low::Util::List<Buffer> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);
      }

      bool Buffer::is_alive() const
      {
        return m_Data.m_Type == Buffer::TYPE_ID &&
               check_alive(ms_Slots, Buffer::get_capacity());
      }

      uint32_t Buffer::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity =
              Low::Util::Config::get_capacity(N(LowRenderer), N(Buffer));
        }
        return l_Capacity;
      }

      Backend::Buffer &Buffer::get_buffer() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Buffer, buffer, Backend::Buffer);
      }

      Low::Util::Name Buffer::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Buffer, name, Low::Util::Name);
      }
      void Buffer::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Buffer, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      Buffer Buffer::make(Util::Name p_Name, BufferCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        Buffer l_Buffer = Buffer::make(p_Name);

        Backend::BufferCreateParams l_Params;
        l_Params.context = &(p_Params.context.get_context());
        l_Params.commandPool = &(p_Params.commandPool.get_commandpool());
        l_Params.bufferSize = p_Params.bufferSize;
        l_Params.data = p_Params.data;

        Backend::buffer_create(l_Buffer.get_buffer(), l_Params);

        return l_Buffer;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low