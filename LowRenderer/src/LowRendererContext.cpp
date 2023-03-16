#include "LowRendererContext.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t Context::TYPE_ID = 4;
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

        ContextData *l_DataPtr =
            (ContextData *)&ms_Buffer[l_Index * sizeof(ContextData)];
        new (l_DataPtr) ContextData();

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
        Backend::callbacks().context_cleanup(get_context());
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

        LOW_PROFILE_ALLOC(type_buffer_Context);
        LOW_PROFILE_ALLOC(type_slots_Context);
      }

      void Context::cleanup()
      {
        Low::Util::List<Context> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_Context);
        LOW_PROFILE_FREE(type_slots_Context);
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
          l_Capacity =
              Low::Util::Config::get_capacity(N(LowRenderer), N(Context));
        }
        return l_Capacity;
      }

      Backend::Context &Context::get_context() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Context, context, Backend::Context);
      }

      Util::List<Renderpass> &Context::get_renderpasses() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Context, renderpasses, Util::List<Renderpass>);
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

      Context Context::make(Util::Name p_Name, Window *p_Window,
                            uint8_t p_FramesInFlight, bool p_ValidationEnabled)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        Backend::ContextCreateParams l_Params;
        l_Params.window = p_Window;
        l_Params.validation_enabled = p_ValidationEnabled;
        l_Params.framesInFlight = p_FramesInFlight;

        Context l_Context = Context::make(p_Name);
        Backend::callbacks().context_create(l_Context.get_context(), l_Params);

        l_Context.get_renderpasses().resize(l_Context.get_image_count());
        for (uint8_t i = 0u; i < l_Context.get_renderpasses().size(); ++i) {
          l_Context.get_renderpasses()[i] = Renderpass::make(p_Name);
          l_Context.get_renderpasses()[i].set_renderpass(
              l_Context.get_context().renderpasses[i]);
        }

        return l_Context;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      uint8_t Context::get_frames_in_flight()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_frames_in_flight
        return get_context().framesInFlight;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_frames_in_flight
      }

      uint8_t Context::get_image_count()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_image_count
        return get_context().imageCount;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_image_count
      }

      uint8_t Context::get_current_frame_index()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_current_frame_index
        return get_context().currentFrameIndex;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_current_frame_index
      }

      uint8_t Context::get_current_image_index()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_current_image_index
        return get_context().currentImageIndex;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_current_image_index
      }

      Renderpass Context::get_current_renderpass()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_current_renderpass
        return get_renderpasses()[get_current_image_index()];
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_current_renderpass
      }

      Math::UVector2 &Context::get_dimensions()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_dimensions
        return get_context().dimensions;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_dimensions
      }

      uint8_t Context::get_image_format()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_image_format
        return get_context().imageFormat;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_image_format
      }

      Window &Context::get_window()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_window
        return get_context().window;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_window
      }

      void Context::wait_idle()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_wait_idle
        Backend::callbacks().context_wait_idle(get_context());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_wait_idle
      }

      uint8_t Context::prepare_frame()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_prepare_frame
        return Backend::callbacks().frame_prepare(get_context());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_prepare_frame
      }

      void Context::render_frame()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_render_frame
        Backend::callbacks().frame_render(get_context());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_render_frame
      }

      void Context::update_dimensions()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_update_dimensions
        Backend::callbacks().context_update_dimensions(get_context());

        get_renderpasses().resize(get_image_count());
        for (uint8_t i = 0u; i < get_renderpasses().size(); ++i) {
          get_renderpasses()[i].set_renderpass(get_context().renderpasses[i]);
        }
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_update_dimensions
      }

      void Context::clear_committed_resource_signatures()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_clear_committed_resource_signatures
        Backend::callbacks().pipeline_resource_signature_commit_clear(
            get_context());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_clear_committed_resource_signatures
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
