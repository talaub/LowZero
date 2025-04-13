#include "LowRendererContext.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowRendererInterface.h"
#include "LowRendererTexture2D.h"
#include "LowRendererMaterial.h"
#include "LowUtilResource.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

#include "LowUtil.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t Context::TYPE_ID = 1;
      uint32_t Context::ms_Capacity = 0u;
      uint8_t *Context::ms_Buffer = 0;
      std::shared_mutex Context::ms_BufferMutex;
      Low::Util::Instances::Slot *Context::ms_Slots = 0;
      Low::Util::List<Context> Context::ms_LivingInstances =
          Low::Util::List<Context>();

      Context::Context() : Low::Util::Handle(0ull)
      {
      }
      Context::Context(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      Context::Context(Context &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle Context::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Context Context::make(Low::Util::Name p_Name)
      {
        WRITE_LOCK(l_Lock);
        uint32_t l_Index = create_instance();

        Context l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Context::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, Context, context,
                                Backend::Context)) Backend::Context();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Context, renderpasses,
                                Util::List<Renderpass>))
            Util::List<Renderpass>();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Context, global_signature,
                                PipelineResourceSignature))
            PipelineResourceSignature();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Context, frame_info_buffer,
                                Resource::Buffer)) Resource::Buffer();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Context,
                                material_data_buffer,
                                Resource::Buffer)) Resource::Buffer();
        ACCESSOR_TYPE_SOA(l_Handle, Context, name, Low::Util::Name) =
            Low::Util::Name(0u);
        LOCK_UNLOCK(l_Lock);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Context::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

        Backend::callbacks().context_cleanup(get_context());
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        WRITE_LOCK(l_Lock);
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
      }

      void Context::initialize()
      {
        WRITE_LOCK(l_Lock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                      N(Context));

        initialize_buffer(&ms_Buffer, ContextData::get_size(),
                          get_capacity(), &ms_Slots);
        LOCK_UNLOCK(l_Lock);

        LOW_PROFILE_ALLOC(type_buffer_Context);
        LOW_PROFILE_ALLOC(type_slots_Context);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Context);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Context::is_alive;
        l_TypeInfo.destroy = &Context::destroy;
        l_TypeInfo.serialize = &Context::serialize;
        l_TypeInfo.deserialize = &Context::deserialize;
        l_TypeInfo.find_by_index = &Context::_find_by_index;
        l_TypeInfo.find_by_name = &Context::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Context::_make;
        l_TypeInfo.duplicate_default = &Context::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Context::living_instances);
        l_TypeInfo.get_living_count = &Context::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: context
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(context);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ContextData, context);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Context l_Handle = p_Handle.get_id();
            l_Handle.get_context();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Context, context, Backend::Context);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Context l_Handle = p_Handle.get_id();
            *((Backend::Context *)p_Data) = l_Handle.get_context();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: context
        }
        {
          // Property: renderpasses
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(renderpasses);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ContextData, renderpasses);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Context l_Handle = p_Handle.get_id();
            l_Handle.get_renderpasses();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Context,
                                              renderpasses,
                                              Util::List<Renderpass>);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Context l_Handle = p_Handle.get_id();
            *((Util::List<Renderpass> *)p_Data) =
                l_Handle.get_renderpasses();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: renderpasses
        }
        {
          // Property: global_signature
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(global_signature);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ContextData, global_signature);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              PipelineResourceSignature::TYPE_ID;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Context l_Handle = p_Handle.get_id();
            l_Handle.get_global_signature();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Context, global_signature,
                PipelineResourceSignature);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Context l_Handle = p_Handle.get_id();
            *((PipelineResourceSignature *)p_Data) =
                l_Handle.get_global_signature();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: global_signature
        }
        {
          // Property: frame_info_buffer
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(frame_info_buffer);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ContextData, frame_info_buffer);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Resource::Buffer::TYPE_ID;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Context l_Handle = p_Handle.get_id();
            l_Handle.get_frame_info_buffer();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Context,
                                              frame_info_buffer,
                                              Resource::Buffer);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Context l_Handle = p_Handle.get_id();
            *((Resource::Buffer *)p_Data) =
                l_Handle.get_frame_info_buffer();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: frame_info_buffer
        }
        {
          // Property: material_data_buffer
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(material_data_buffer);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ContextData, material_data_buffer);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Resource::Buffer::TYPE_ID;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Context l_Handle = p_Handle.get_id();
            l_Handle.get_material_data_buffer();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Context,
                                              material_data_buffer,
                                              Resource::Buffer);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Context l_Handle = p_Handle.get_id();
            *((Resource::Buffer *)p_Data) =
                l_Handle.get_material_data_buffer();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: material_data_buffer
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ContextData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Context l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Context, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Context l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Context l_Handle = p_Handle.get_id();
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Function: make
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(make);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType = Context::TYPE_ID;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Name);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::NAME;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Window);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_FramesInFlight);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_ValidationEnabled);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::BOOL;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: make
        }
        {
          // Function: get_frames_in_flight
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_frames_in_flight);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_frames_in_flight
        }
        {
          // Function: get_image_count
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_image_count);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_image_count
        }
        {
          // Function: get_current_frame_index
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_current_frame_index);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_current_frame_index
        }
        {
          // Function: get_current_image_index
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_current_image_index);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_current_image_index
        }
        {
          // Function: get_current_renderpass
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_current_renderpass);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType = Renderpass::TYPE_ID;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_current_renderpass
        }
        {
          // Function: get_dimensions
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_dimensions);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_dimensions
        }
        {
          // Function: get_image_format
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_image_format);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_image_format
        }
        {
          // Function: get_window
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_window);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_window
        }
        {
          // Function: get_state
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_state);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_state
        }
        {
          // Function: is_debug_enabled
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(is_debug_enabled);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: is_debug_enabled
        }
        {
          // Function: wait_idle
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(wait_idle);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: wait_idle
        }
        {
          // Function: prepare_frame
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(prepare_frame);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: prepare_frame
        }
        {
          // Function: render_frame
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(render_frame);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: render_frame
        }
        {
          // Function: begin_imgui_frame
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(begin_imgui_frame);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: begin_imgui_frame
        }
        {
          // Function: render_imgui
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(render_imgui);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: render_imgui
        }
        {
          // Function: update_dimensions
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(update_dimensions);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: update_dimensions
        }
        {
          // Function: clear_committed_resource_signatures
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name =
              N(clear_committed_resource_signatures);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: clear_committed_resource_signatures
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void Context::cleanup()
      {
        Low::Util::List<Context> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        WRITE_LOCK(l_Lock);
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_Context);
        LOW_PROFILE_FREE(type_slots_Context);
        LOCK_UNLOCK(l_Lock);
      }

      Low::Util::Handle Context::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Context Context::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Context l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = Context::TYPE_ID;

        return l_Handle;
      }

      bool Context::is_alive() const
      {
        READ_LOCK(l_Lock);
        return m_Data.m_Type == Context::TYPE_ID &&
               check_alive(ms_Slots, Context::get_capacity());
      }

      uint32_t Context::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle Context::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      Context Context::find_by_name(Low::Util::Name p_Name)
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
        return 0ull;
      }

      Context Context::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        Context l_Handle = make(p_Name);
        if (get_global_signature().is_alive()) {
          l_Handle.set_global_signature(get_global_signature());
        }
        if (get_frame_info_buffer().is_alive()) {
          l_Handle.set_frame_info_buffer(get_frame_info_buffer());
        }
        if (get_material_data_buffer().is_alive()) {
          l_Handle.set_material_data_buffer(
              get_material_data_buffer());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      Context Context::duplicate(Context p_Handle,
                                 Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      Context::_duplicate(Low::Util::Handle p_Handle,
                          Low::Util::Name p_Name)
      {
        Context l_Context = p_Handle.get_id();
        return l_Context.duplicate(p_Name);
      }

      void Context::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        if (get_global_signature().is_alive()) {
          get_global_signature().serialize(
              p_Node["global_signature"]);
        }
        if (get_frame_info_buffer().is_alive()) {
          get_frame_info_buffer().serialize(
              p_Node["frame_info_buffer"]);
        }
        if (get_material_data_buffer().is_alive()) {
          get_material_data_buffer().serialize(
              p_Node["material_data_buffer"]);
        }
        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Context::serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node)
      {
        Context l_Context = p_Handle.get_id();
        l_Context.serialize(p_Node);
      }

      Low::Util::Handle
      Context::deserialize(Low::Util::Yaml::Node &p_Node,
                           Low::Util::Handle p_Creator)
      {
        Context l_Handle = Context::make(N(Context));

        if (p_Node["context"]) {
        }
        if (p_Node["renderpasses"]) {
        }
        if (p_Node["global_signature"]) {
          l_Handle.set_global_signature(
              PipelineResourceSignature::deserialize(
                  p_Node["global_signature"], l_Handle.get_id())
                  .get_id());
        }
        if (p_Node["frame_info_buffer"]) {
          l_Handle.set_frame_info_buffer(
              Resource::Buffer::deserialize(
                  p_Node["frame_info_buffer"], l_Handle.get_id())
                  .get_id());
        }
        if (p_Node["material_data_buffer"]) {
          l_Handle.set_material_data_buffer(
              Resource::Buffer::deserialize(
                  p_Node["material_data_buffer"], l_Handle.get_id())
                  .get_id());
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      Backend::Context &Context::get_context() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_context

        // LOW_CODEGEN::END::CUSTOM:GETTER_context

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(Context, context, Backend::Context);
      }

      Util::List<Renderpass> &Context::get_renderpasses() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_renderpasses

        // LOW_CODEGEN::END::CUSTOM:GETTER_renderpasses

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(Context, renderpasses,
                        Util::List<Renderpass>);
      }

      PipelineResourceSignature Context::get_global_signature() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_global_signature

        // LOW_CODEGEN::END::CUSTOM:GETTER_global_signature

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(Context, global_signature,
                        PipelineResourceSignature);
      }
      void
      Context::set_global_signature(PipelineResourceSignature p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_global_signature

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_global_signature

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(Context, global_signature,
                 PipelineResourceSignature) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_global_signature

        // LOW_CODEGEN::END::CUSTOM:SETTER_global_signature
      }

      Resource::Buffer Context::get_frame_info_buffer() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_frame_info_buffer

        // LOW_CODEGEN::END::CUSTOM:GETTER_frame_info_buffer

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(Context, frame_info_buffer, Resource::Buffer);
      }
      void Context::set_frame_info_buffer(Resource::Buffer p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_frame_info_buffer

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_frame_info_buffer

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(Context, frame_info_buffer, Resource::Buffer) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_frame_info_buffer

        // LOW_CODEGEN::END::CUSTOM:SETTER_frame_info_buffer
      }

      Resource::Buffer Context::get_material_data_buffer() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material_data_buffer

        // LOW_CODEGEN::END::CUSTOM:GETTER_material_data_buffer

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(Context, material_data_buffer,
                        Resource::Buffer);
      }
      void Context::set_material_data_buffer(Resource::Buffer p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material_data_buffer

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_material_data_buffer

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(Context, material_data_buffer, Resource::Buffer) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material_data_buffer

        // LOW_CODEGEN::END::CUSTOM:SETTER_material_data_buffer
      }

      Low::Util::Name Context::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(Context, name, Low::Util::Name);
      }
      void Context::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(Context, name, Low::Util::Name) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      Context Context::make(Util::Name p_Name, Window *p_Window,
                            uint8_t p_FramesInFlight,
                            bool p_ValidationEnabled)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make

        Backend::ContextCreateParams l_Params;
        l_Params.window = p_Window;
        l_Params.validation_enabled = p_ValidationEnabled;
        l_Params.framesInFlight = p_FramesInFlight;

        Context l_Context = Context::make(p_Name);
        Backend::callbacks().context_create(l_Context.get_context(),
                                            l_Params);

        l_Context.get_renderpasses().resize(
            l_Context.get_image_count());
        for (uint8_t i = 0u; i < l_Context.get_renderpasses().size();
             ++i) {
          l_Context.get_renderpasses()[i] = Renderpass::make(p_Name);
          l_Context.get_renderpasses()[i].set_renderpass(
              l_Context.get_context().renderpasses[i]);
        }

        {
          Backend::BufferCreateParams l_Params;
          l_Params.bufferSize = sizeof(Math::Vector2);
          l_Params.context = &l_Context.get_context();
          l_Params.usageFlags =
              LOW_RENDERER_BUFFER_USAGE_RESOURCE_CONSTANT;

          Math::UVector2 l_ContextDimensions = {1, 1};
          if (l_Context.get_dimensions().x > 0) {
            l_ContextDimensions.x = l_ContextDimensions.x;
          }
          if (l_Context.get_dimensions().y > 0) {
            l_ContextDimensions.y = l_ContextDimensions.y;
          }

          Math::Vector2 l_InverseDimensions = {
              1.0f / ((float)l_ContextDimensions.x),
              1.0f / ((float)l_ContextDimensions.y)};

          l_Params.data = &l_InverseDimensions;

          l_Context.set_frame_info_buffer(Resource::Buffer::make(
              N(ContextFrameInfoBuffer), l_Params));
        }

        {
          Backend::BufferCreateParams l_Params;
          l_Params.bufferSize = Material::get_capacity() *
                                (sizeof(Math::Vector4) *
                                 LOW_RENDERER_MATERIAL_DATA_VECTORS);
          l_Params.context = &l_Context.get_context();
          l_Params.usageFlags =
              LOW_RENDERER_BUFFER_USAGE_RESOURCE_BUFFER;
          l_Params.data = nullptr;

          l_Context.set_material_data_buffer(Resource::Buffer::make(
              N(ContextMaterialDataBuffer), l_Params));
        }

        {
          Util::List<Backend::PipelineResourceDescription>
              l_Resources;

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

          l_Context.set_global_signature(
              PipelineResourceSignature::make(
                  N(GlobalSignature), l_Context, 0, l_Resources));

          l_Context.get_global_signature()
              .set_constant_buffer_resource(
                  N(g_ContextFrameInfo), 0,
                  l_Context.get_frame_info_buffer());

          l_Context.get_global_signature().set_buffer_resource(
              N(g_MaterialInfos), 0,
              l_Context.get_material_data_buffer());

          {
            Util::Resource::Image2D l_Image;
            Util::Resource::load_image2d(
                Util::get_project().dataPath +
                    "/resources/img2d/default_texture.ktx",
                l_Image);

            Texture2D l_Texture2D = Texture2D::make(
                N(DefaultTexture), l_Context, l_Image);

            for (uint32_t i = 1u; i < Texture2D::get_capacity();
                 ++i) {
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

        LOW_PROFILE_CPU("Renderer", "Prepare frame");
        return Backend::callbacks().frame_prepare(get_context());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_prepare_frame
      }

      void Context::render_frame()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_render_frame

        LOW_PROFILE_CPU("Renderer", "Render frame");
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

        Backend::callbacks().context_wait_idle(get_context());
        Backend::callbacks().context_update_dimensions(get_context());

        get_renderpasses().resize(get_image_count());
        for (uint8_t i = 0u; i < get_renderpasses().size(); ++i) {
          get_renderpasses()[i].set_renderpass(
              get_context().renderpasses[i]);
        }

        Math::Vector2 l_InverseDimensions = {
            1.0f / ((float)get_dimensions().x),
            1.0f / ((float)get_dimensions().y)};

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

      uint32_t Context::create_instance()
      {
        uint32_t l_Index = 0u;

        for (; l_Index < get_capacity(); ++l_Index) {
          if (!ms_Slots[l_Index].m_Occupied) {
            break;
          }
        }
        if (l_Index >= get_capacity()) {
          increase_budget();
        }
        ms_Slots[l_Index].m_Occupied = true;
        return l_Index;
      }

      void Context::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease =
            std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0,
                   "Could not increase capacity");

        uint8_t *l_NewBuffer = (uint8_t *)malloc(
            (l_Capacity + l_CapacityIncrease) * sizeof(ContextData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(&l_NewBuffer[offsetof(ContextData, context) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ContextData, context) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Backend::Context));
        }
        {
          for (auto it = ms_LivingInstances.begin();
               it != ms_LivingInstances.end(); ++it) {
            Context i_Context = *it;

            auto *i_ValPtr = new (
                &l_NewBuffer[offsetof(ContextData, renderpasses) *
                                 (l_Capacity + l_CapacityIncrease) +
                             (it->get_index() *
                              sizeof(Util::List<Renderpass>))])
                Util::List<Renderpass>();
            *i_ValPtr =
                ACCESSOR_TYPE_SOA(i_Context, Context, renderpasses,
                                  Util::List<Renderpass>);
          }
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(ContextData, global_signature) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(ContextData, global_signature) *
                         (l_Capacity)],
              l_Capacity * sizeof(PipelineResourceSignature));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(ContextData, frame_info_buffer) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(ContextData, frame_info_buffer) *
                         (l_Capacity)],
              l_Capacity * sizeof(Resource::Buffer));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(ContextData,
                                    material_data_buffer) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(ContextData, material_data_buffer) *
                         (l_Capacity)],
              l_Capacity * sizeof(Resource::Buffer));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(ContextData, name) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(ContextData, name) * (l_Capacity)],
              l_Capacity * sizeof(Low::Util::Name));
        }
        for (uint32_t i = l_Capacity;
             i < l_Capacity + l_CapacityIncrease; ++i) {
          l_NewSlots[i].m_Occupied = false;
          l_NewSlots[i].m_Generation = 0;
        }
        free(ms_Buffer);
        free(ms_Slots);
        ms_Buffer = l_NewBuffer;
        ms_Slots = l_NewSlots;
        ms_Capacity = l_Capacity + l_CapacityIncrease;

        LOW_LOG_DEBUG << "Auto-increased budget for Context from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease)
                      << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
