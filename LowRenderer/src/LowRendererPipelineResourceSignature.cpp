#include "LowRendererPipelineResourceSignature.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowRendererInterface.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t PipelineResourceSignature::TYPE_ID = 3;
      uint32_t PipelineResourceSignature::ms_Capacity = 0u;
      uint8_t *PipelineResourceSignature::ms_Buffer = 0;
      std::shared_mutex PipelineResourceSignature::ms_BufferMutex;
      Low::Util::Instances::Slot
          *PipelineResourceSignature::ms_Slots = 0;
      Low::Util::List<PipelineResourceSignature>
          PipelineResourceSignature::ms_LivingInstances =
              Low::Util::List<PipelineResourceSignature>();

      PipelineResourceSignature::PipelineResourceSignature()
          : Low::Util::Handle(0ull)
      {
      }
      PipelineResourceSignature::PipelineResourceSignature(
          uint64_t p_Id)
          : Low::Util::Handle(p_Id)
      {
      }
      PipelineResourceSignature::PipelineResourceSignature(
          PipelineResourceSignature &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle
      PipelineResourceSignature::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      PipelineResourceSignature
      PipelineResourceSignature::make(Low::Util::Name p_Name)
      {
        WRITE_LOCK(l_Lock);
        uint32_t l_Index = create_instance();

        PipelineResourceSignature l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = PipelineResourceSignature::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, PipelineResourceSignature,
                                signature,
                                Backend::PipelineResourceSignature))
            Backend::PipelineResourceSignature();
        ACCESSOR_TYPE_SOA(l_Handle, PipelineResourceSignature, name,
                          Low::Util::Name) = Low::Util::Name(0u);
        LOCK_UNLOCK(l_Lock);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void PipelineResourceSignature::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

        // LOW_CODEGEN::END::CUSTOM:DESTROY

        broadcast_observable(OBSERVABLE_DESTROY);

        WRITE_LOCK(l_Lock);
        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end();) {
          if (it->get_id() == get_id()) {
            it = ms_LivingInstances.erase(it);
          } else {
            it++;
          }
        }
      }

      void PipelineResourceSignature::initialize()
      {
        WRITE_LOCK(l_Lock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(
            N(LowRenderer), N(PipelineResourceSignature));

        initialize_buffer(&ms_Buffer,
                          PipelineResourceSignatureData::get_size(),
                          get_capacity(), &ms_Slots);
        LOCK_UNLOCK(l_Lock);

        LOW_PROFILE_ALLOC(type_buffer_PipelineResourceSignature);
        LOW_PROFILE_ALLOC(type_slots_PipelineResourceSignature);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(PipelineResourceSignature);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &PipelineResourceSignature::is_alive;
        l_TypeInfo.destroy = &PipelineResourceSignature::destroy;
        l_TypeInfo.serialize = &PipelineResourceSignature::serialize;
        l_TypeInfo.deserialize =
            &PipelineResourceSignature::deserialize;
        l_TypeInfo.find_by_index =
            &PipelineResourceSignature::_find_by_index;
        l_TypeInfo.notify = &PipelineResourceSignature::_notify;
        l_TypeInfo.find_by_name =
            &PipelineResourceSignature::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &PipelineResourceSignature::_make;
        l_TypeInfo.duplicate_default =
            &PipelineResourceSignature::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &PipelineResourceSignature::living_instances);
        l_TypeInfo.get_living_count =
            &PipelineResourceSignature::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: signature
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(signature);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(PipelineResourceSignatureData, signature);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            PipelineResourceSignature l_Handle = p_Handle.get_id();
            l_Handle.get_signature();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, PipelineResourceSignature, signature,
                Backend::PipelineResourceSignature);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            PipelineResourceSignature l_Handle = p_Handle.get_id();
            *((Backend::PipelineResourceSignature *)p_Data) =
                l_Handle.get_signature();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: signature
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(PipelineResourceSignatureData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            PipelineResourceSignature l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, PipelineResourceSignature, name,
                Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            PipelineResourceSignature l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            PipelineResourceSignature l_Handle = p_Handle.get_id();
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
          l_FunctionInfo.handleType =
              PipelineResourceSignature::TYPE_ID;
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
            l_ParameterInfo.name = N(p_Context);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Context::TYPE_ID;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Binding);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_ResourceDescriptions);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: make
        }
        {
          // Function: commit
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(commit);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: commit
        }
        {
          // Function: set_image_resource
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(set_image_resource);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
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
            l_ParameterInfo.name = N(p_ArrayIndex);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UINT32;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Value);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Resource::Image::TYPE_ID;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: set_image_resource
        }
        {
          // Function: set_sampler_resource
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(set_sampler_resource);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
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
            l_ParameterInfo.name = N(p_ArrayIndex);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UINT32;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Value);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Resource::Image::TYPE_ID;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: set_sampler_resource
        }
        {
          // Function: set_unbound_sampler_resource
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(set_unbound_sampler_resource);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
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
            l_ParameterInfo.name = N(p_ArrayIndex);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UINT32;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Value);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Resource::Image::TYPE_ID;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: set_unbound_sampler_resource
        }
        {
          // Function: set_texture2d_resource
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(set_texture2d_resource);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
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
            l_ParameterInfo.name = N(p_ArrayIndex);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UINT32;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Value);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Resource::Image::TYPE_ID;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: set_texture2d_resource
        }
        {
          // Function: set_constant_buffer_resource
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(set_constant_buffer_resource);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
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
            l_ParameterInfo.name = N(p_ArrayIndex);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UINT32;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Value);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Resource::Buffer::TYPE_ID;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: set_constant_buffer_resource
        }
        {
          // Function: set_buffer_resource
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(set_buffer_resource);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
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
            l_ParameterInfo.name = N(p_ArrayIndex);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UINT32;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Value);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Resource::Buffer::TYPE_ID;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: set_buffer_resource
        }
        {
          // Function: get_binding
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_binding);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_binding
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void PipelineResourceSignature::cleanup()
      {
        Low::Util::List<PipelineResourceSignature> l_Instances =
            ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        WRITE_LOCK(l_Lock);
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_PipelineResourceSignature);
        LOW_PROFILE_FREE(type_slots_PipelineResourceSignature);
        LOCK_UNLOCK(l_Lock);
      }

      Low::Util::Handle
      PipelineResourceSignature::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      PipelineResourceSignature
      PipelineResourceSignature::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        PipelineResourceSignature l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = PipelineResourceSignature::TYPE_ID;

        return l_Handle;
      }

      PipelineResourceSignature
      PipelineResourceSignature::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        PipelineResourceSignature l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = PipelineResourceSignature::TYPE_ID;

        return l_Handle;
      }

      bool PipelineResourceSignature::is_alive() const
      {
        READ_LOCK(l_Lock);
        return m_Data.m_Type == PipelineResourceSignature::TYPE_ID &&
               check_alive(ms_Slots,
                           PipelineResourceSignature::get_capacity());
      }

      uint32_t PipelineResourceSignature::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      PipelineResourceSignature::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      PipelineResourceSignature
      PipelineResourceSignature::find_by_name(Low::Util::Name p_Name)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:FIND_BY_NAME
        // LOW_CODEGEN::END::CUSTOM:FIND_BY_NAME

        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
        return Low::Util::Handle::DEAD;
      }

      PipelineResourceSignature PipelineResourceSignature::duplicate(
          Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        PipelineResourceSignature l_Handle = make(p_Name);

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      PipelineResourceSignature PipelineResourceSignature::duplicate(
          PipelineResourceSignature p_Handle, Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle PipelineResourceSignature::_duplicate(
          Low::Util::Handle p_Handle, Low::Util::Name p_Name)
      {
        PipelineResourceSignature l_PipelineResourceSignature =
            p_Handle.get_id();
        return l_PipelineResourceSignature.duplicate(p_Name);
      }

      void PipelineResourceSignature::serialize(
          Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void PipelineResourceSignature::serialize(
          Low::Util::Handle p_Handle, Low::Util::Yaml::Node &p_Node)
      {
        PipelineResourceSignature l_PipelineResourceSignature =
            p_Handle.get_id();
        l_PipelineResourceSignature.serialize(p_Node);
      }

      Low::Util::Handle PipelineResourceSignature::deserialize(
          Low::Util::Yaml::Node &p_Node, Low::Util::Handle p_Creator)
      {
        PipelineResourceSignature l_Handle =
            PipelineResourceSignature::make(
                N(PipelineResourceSignature));

        if (p_Node["signature"]) {
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void PipelineResourceSignature::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 PipelineResourceSignature::observe(
          Low::Util::Name p_Observable,
          Low::Util::Function<void(Low::Util::Handle,
                                   Low::Util::Name)>
              p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 PipelineResourceSignature::observe(
          Low::Util::Name p_Observable,
          Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void
      PipelineResourceSignature::notify(Low::Util::Handle p_Observed,
                                        Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void
      PipelineResourceSignature::_notify(Low::Util::Handle p_Observer,
                                         Low::Util::Handle p_Observed,
                                         Low::Util::Name p_Observable)
      {
        PipelineResourceSignature l_PipelineResourceSignature =
            p_Observer.get_id();
        l_PipelineResourceSignature.notify(p_Observed, p_Observable);
      }

      Backend::PipelineResourceSignature &
      PipelineResourceSignature::get_signature() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_signature

        // LOW_CODEGEN::END::CUSTOM:GETTER_signature

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(PipelineResourceSignature, signature,
                        Backend::PipelineResourceSignature);
      }

      Low::Util::Name PipelineResourceSignature::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(PipelineResourceSignature, name,
                        Low::Util::Name);
      }
      void
      PipelineResourceSignature::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(PipelineResourceSignature, name, Low::Util::Name) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      PipelineResourceSignature PipelineResourceSignature::make(
          Util::Name p_Name, Context p_Context, uint8_t p_Binding,
          Util::List<Backend::PipelineResourceDescription>
              &p_ResourceDescriptions)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make

        PipelineResourceSignature l_Signature =
            PipelineResourceSignature::make(p_Name);

        Backend::PipelineResourceSignatureCreateParams l_Params;
        l_Params.context = &p_Context.get_context();
        l_Params.binding = p_Binding;
        l_Params.resourceDescriptionCount =
            p_ResourceDescriptions.size();
        l_Params.resourceDescriptions = p_ResourceDescriptions.data();

        Backend::callbacks().pipeline_resource_signature_create(
            l_Signature.get_signature(), l_Params);

        return l_Signature;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      void PipelineResourceSignature::commit()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_commit

        Backend::callbacks().pipeline_resource_signature_commit(
            get_signature());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_commit
      }

      void PipelineResourceSignature::set_image_resource(
          Util::Name p_Name, uint32_t p_ArrayIndex,
          Resource::Image p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_image_resource

        Backend::callbacks().pipeline_resource_signature_set_image(
            get_signature(), p_Name, p_ArrayIndex, p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_image_resource
      }

      void PipelineResourceSignature::set_sampler_resource(
          Util::Name p_Name, uint32_t p_ArrayIndex,
          Resource::Image p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_sampler_resource

        Backend::callbacks().pipeline_resource_signature_set_sampler(
            get_signature(), p_Name, p_ArrayIndex, p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_sampler_resource
      }

      void PipelineResourceSignature::set_unbound_sampler_resource(
          Util::Name p_Name, uint32_t p_ArrayIndex,
          Resource::Image p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_unbound_sampler_resource

        Backend::callbacks()
            .pipeline_resource_signature_set_unbound_sampler(
                get_signature(), p_Name, p_ArrayIndex, p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_unbound_sampler_resource
      }

      void PipelineResourceSignature::set_texture2d_resource(
          Util::Name p_Name, uint32_t p_ArrayIndex,
          Resource::Image p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_texture2d_resource

        Backend::callbacks()
            .pipeline_resource_signature_set_texture2d(
                get_signature(), p_Name, p_ArrayIndex, p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_texture2d_resource
      }

      void PipelineResourceSignature::set_constant_buffer_resource(
          Util::Name p_Name, uint32_t p_ArrayIndex,
          Resource::Buffer p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_constant_buffer_resource

        Backend::callbacks()
            .pipeline_resource_signature_set_constant_buffer(
                get_signature(), p_Name, p_ArrayIndex, p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_constant_buffer_resource
      }

      void PipelineResourceSignature::set_buffer_resource(
          Util::Name p_Name, uint32_t p_ArrayIndex,
          Resource::Buffer p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_buffer_resource

        Backend::callbacks().pipeline_resource_signature_set_buffer(
            get_signature(), p_Name, p_ArrayIndex, p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_buffer_resource
      }

      uint8_t PipelineResourceSignature::get_binding()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_binding

        return get_signature().binding;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_binding
      }

      uint32_t PipelineResourceSignature::create_instance()
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

      void PipelineResourceSignature::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease =
            std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0,
                   "Could not increase capacity");

        uint8_t *l_NewBuffer =
            (uint8_t *)malloc((l_Capacity + l_CapacityIncrease) *
                              sizeof(PipelineResourceSignatureData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(&l_NewBuffer[offsetof(PipelineResourceSignatureData,
                                       signature) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(PipelineResourceSignatureData,
                                     signature) *
                            (l_Capacity)],
                 l_Capacity *
                     sizeof(Backend::PipelineResourceSignature));
        }
        {
          memcpy(&l_NewBuffer[offsetof(PipelineResourceSignatureData,
                                       name) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(PipelineResourceSignatureData,
                                     name) *
                            (l_Capacity)],
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

        LOW_LOG_DEBUG << "Auto-increased budget for "
                         "PipelineResourceSignature from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease)
                      << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Interface
  } // namespace Renderer
} // namespace Low
