#include "LowRendererContext.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"
#include "LowRendererTexture2D.h"
#include "LowRendererMaterial.h"
#include "LowUtilResource.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t Context::TYPE_ID = 11;
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

        new (&ACCESSOR_TYPE_SOA(l_Handle, Context, context, Backend::Context))
            Backend::Context();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Context, renderpasses,
                                Util::List<Renderpass>))
            Util::List<Renderpass>();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Context, global_signature,
                                PipelineResourceSignature))
            PipelineResourceSignature();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Context, frame_info_buffer,
                                Resource::Buffer)) Resource::Buffer();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Context, material_data_buffer,
                                Resource::Buffer)) Resource::Buffer();
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

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Context);
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Context::is_alive;
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(context);
          l_PropertyInfo.dataOffset = offsetof(ContextData, context);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Context, context,
                                              Backend::Context);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Context, context, Backend::Context) =
                *(Backend::Context *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(renderpasses);
          l_PropertyInfo.dataOffset = offsetof(ContextData, renderpasses);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Context, renderpasses,
                                              Util::List<Renderpass>);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Context, renderpasses,
                              Util::List<Renderpass>) =
                *(Util::List<Renderpass> *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(global_signature);
          l_PropertyInfo.dataOffset = offsetof(ContextData, global_signature);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Context, global_signature, PipelineResourceSignature);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Context, global_signature,
                              PipelineResourceSignature) =
                *(PipelineResourceSignature *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(frame_info_buffer);
          l_PropertyInfo.dataOffset = offsetof(ContextData, frame_info_buffer);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Context, frame_info_buffer, Resource::Buffer);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Context, frame_info_buffer,
                              Resource::Buffer) = *(Resource::Buffer *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(material_data_buffer);
          l_PropertyInfo.dataOffset =
              offsetof(ContextData, material_data_buffer);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Context, material_data_buffer, Resource::Buffer);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Context, material_data_buffer,
                              Resource::Buffer) = *(Resource::Buffer *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.dataOffset = offsetof(ContextData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Context, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Context, name, Low::Util::Name) =
                *(Low::Util::Name *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
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

      PipelineResourceSignature Context::get_global_signature() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Context, global_signature, PipelineResourceSignature);
      }
      void Context::set_global_signature(PipelineResourceSignature p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Context, global_signature, PipelineResourceSignature) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_global_signature
        // LOW_CODEGEN::END::CUSTOM:SETTER_global_signature
      }

      Resource::Buffer Context::get_frame_info_buffer() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Context, frame_info_buffer, Resource::Buffer);
      }
      void Context::set_frame_info_buffer(Resource::Buffer p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Context, frame_info_buffer, Resource::Buffer) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_frame_info_buffer
        // LOW_CODEGEN::END::CUSTOM:SETTER_frame_info_buffer
      }

      Resource::Buffer Context::get_material_data_buffer() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Context, material_data_buffer, Resource::Buffer);
      }
      void Context::set_material_data_buffer(Resource::Buffer p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Context, material_data_buffer, Resource::Buffer) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material_data_buffer
        // LOW_CODEGEN::END::CUSTOM:SETTER_material_data_buffer
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

        {
          Backend::BufferCreateParams l_Params;
          l_Params.bufferSize = sizeof(Math::Vector2);
          l_Params.context = &l_Context.get_context();
          l_Params.usageFlags = LOW_RENDERER_BUFFER_USAGE_RESOURCE_CONSTANT;

          Math::Vector2 l_InverseDimensions = {
              1.0f / ((float)l_Context.get_dimensions().x),
              1.0f / ((float)l_Context.get_dimensions().y)};

          l_Params.data = &l_InverseDimensions;

          l_Context.set_frame_info_buffer(
              Resource::Buffer::make(N(ContextFrameInfoBuffer), l_Params));
        }

        {
          Backend::BufferCreateParams l_Params;
          l_Params.bufferSize =
              Material::get_capacity() *
              (sizeof(Math::Vector4) * LOW_RENDERER_MATERIAL_DATA_VECTORS);
          l_Params.context = &l_Context.get_context();
          l_Params.usageFlags = LOW_RENDERER_BUFFER_USAGE_RESOURCE_BUFFER;
          l_Params.data = nullptr;

          l_Context.set_material_data_buffer(
              Resource::Buffer::make(N(ContextMaterialDataBuffer), l_Params));
        }

        {
          Util::List<Backend::PipelineResourceDescription> l_Resources;

          {
            Backend::PipelineResourceDescription l_Resource;
            l_Resource.name = N(g_ContextFrameInfo);
            l_Resource.step = Backend::ResourcePipelineStep::ALL;
            l_Resource.type = Backend::ResourceType::CONSTANT_BUFFER;
            l_Resource.arraySize = 1;
            l_Resources.push_back(l_Resource);
          }

          {
            Backend::PipelineResourceDescription l_Resource;
            l_Resource.name = N(g_MaterialInfos);
            l_Resource.step = Backend::ResourcePipelineStep::ALL;
            l_Resource.type = Backend::ResourceType::BUFFER;
            l_Resource.arraySize = 1;
            l_Resources.push_back(l_Resource);
          }

          {
            Backend::PipelineResourceDescription l_Resource;
            l_Resource.name = N(g_Texture2Ds);
            l_Resource.step = Backend::ResourcePipelineStep::ALL;
            l_Resource.type = Backend::ResourceType::SAMPLER;
            l_Resource.arraySize = Texture2D::get_capacity();
            l_Resources.push_back(l_Resource);
          }

          l_Context.set_global_signature(PipelineResourceSignature::make(
              N(GlobalSignature), l_Context, 0, l_Resources));

          l_Context.get_global_signature().set_constant_buffer_resource(
              N(g_ContextFrameInfo), 0, l_Context.get_frame_info_buffer());

          l_Context.get_global_signature().set_buffer_resource(
              N(g_MaterialInfos), 0, l_Context.get_material_data_buffer());

          {
            Util::Resource::Image2D l_Image;
            Util::Resource::load_image2d(
                Util::String(LOW_DATA_PATH) +
                    "/assets/img2d/default_texture.ktx",
                l_Image);

            Texture2D l_Texture2D =
                Texture2D::make(N(DefaultTexture), l_Context, l_Image);

            for (uint32_t i = 1u; i < Texture2D::get_capacity(); ++i) {
              l_Context.get_global_signature().set_sampler_resource(
                  N(g_Texture2Ds), i, l_Texture2D.get_image());
            }
          }
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

      uint8_t Context::get_state()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_state
        return get_context().state;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_state
      }

      bool Context::is_debug_enabled()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_debug_enabled
        return get_context().debugEnabled;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_debug_enabled
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

      void Context::begin_imgui_frame()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_begin_imgui_frame
        Backend::callbacks().imgui_prepare_frame(get_context());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_begin_imgui_frame
      }

      void Context::render_imgui()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_render_imgui
        Backend::callbacks().imgui_render(get_context());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_render_imgui
      }

      void Context::update_dimensions()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_update_dimensions
        Backend::callbacks().context_update_dimensions(get_context());

        get_renderpasses().resize(get_image_count());
        for (uint8_t i = 0u; i < get_renderpasses().size(); ++i) {
          get_renderpasses()[i].set_renderpass(get_context().renderpasses[i]);
        }

        Math::Vector2 l_InverseDimensions = {1.0f / ((float)get_dimensions().x),
                                             1.0f /
                                                 ((float)get_dimensions().y)};

        get_frame_info_buffer().set(&l_InverseDimensions);

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
