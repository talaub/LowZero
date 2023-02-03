#include "LowRendererContext.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t Context::TYPE_ID = 1;
      uint8_t *Context::ms_Buffer = 0;
      Low::Util::Instances::Slot *Context::ms_Slots = 0;
      Low::Util::List<Context> Context::ms_LivingInstances =
          Low::Util::List<Context>();

      Context::Context() : Low::Util::Handle(0ull)
      {
      }
      Context::Context(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      Context::Context(Context &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Context Context::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        Context l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Context::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, Context, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void Context::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        Backend::context_cleanup(get_context());
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const Context *l_Instances = living_instances();
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

      void Context::initialize()
      {
        initialize_buffer(&ms_Buffer, ContextData::get_size(), get_capacity(),
                          &ms_Slots);
      }

      void Context::cleanup()
      {
        Low::Util::List<Context> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);
      }

      bool Context::is_alive() const
      {
        return m_Data.m_Type == Context::TYPE_ID &&
               check_alive(ms_Slots, Context::get_capacity());
      }

      uint32_t Context::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity = Low::Util::Config::get_capacity(N(Context));
        }
        return l_Capacity;
      }

      Low::Renderer::Backend::Context &Context::get_context() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Context, context, Low::Renderer::Backend::Context);
      }

      Low::Util::Name Context::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Context, name, Low::Util::Name);
      }
      void Context::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Context, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      Context Context::make(Util::Name p_Name, ContextCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        Context l_Context = Context::make(p_Name);

        Backend::ContextCreateParams l_Params;
        l_Params.window = p_Params.window;
        l_Params.validation_enabled = p_Params.validation_enabled;

        Backend::context_create(l_Context.get_context(), l_Params);

        return l_Context;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      void Context::wait_idle()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_wait_idle
        Backend::context_wait_idle(get_context());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_wait_idle
      }

      Window &Context::get_window()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_window
        return get_context().m_Window;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_window
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
