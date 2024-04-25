#include "LowRendererBuffer.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    namespace Resource {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t Buffer::TYPE_ID = 8;
      uint32_t Buffer::ms_Capacity = 0u;
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

      Low::Util::Handle Buffer::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Buffer Buffer::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = create_instance();

        Buffer l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Buffer::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, Buffer, buffer,
                                Backend::Buffer)) Backend::Buffer();
        ACCESSOR_TYPE_SOA(l_Handle, Buffer, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Buffer::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        Backend::callbacks().buffer_cleanup(get_buffer());
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
      }

      void Buffer::initialize()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                      N(Buffer));

        initialize_buffer(&ms_Buffer, BufferData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_Buffer);
        LOW_PROFILE_ALLOC(type_slots_Buffer);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Buffer);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Buffer::is_alive;
        l_TypeInfo.destroy = &Buffer::destroy;
        l_TypeInfo.serialize = &Buffer::serialize;
        l_TypeInfo.deserialize = &Buffer::deserialize;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Buffer::_make;
        l_TypeInfo.duplicate_default = &Buffer::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Buffer::living_instances);
        l_TypeInfo.get_living_count = &Buffer::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(buffer);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(BufferData, buffer);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Buffer l_Handle = p_Handle.get_id();
            l_Handle.get_buffer();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Buffer, buffer, Backend::Buffer);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Buffer l_Handle = p_Handle.get_id();
            l_Handle.set_buffer(*(Backend::Buffer *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(BufferData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Buffer l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Buffer, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Buffer l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(make);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType = Buffer::TYPE_ID;
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
            l_ParameterInfo.name = N(p_Params);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        }
        {
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(set);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Data);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        }
        {
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(write);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Data);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_DataSize);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UINT32;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Start);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UINT32;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        }
        {
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(read);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Data);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_DataSize);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UINT32;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Start);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UINT32;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        }
        {
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(bind_vertex);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        }
        {
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(bind_index);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_BindType);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void Buffer::cleanup()
      {
        Low::Util::List<Buffer> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_Buffer);
        LOW_PROFILE_FREE(type_slots_Buffer);
      }

      Buffer Buffer::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Buffer l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = Buffer::TYPE_ID;

        return l_Handle;
      }

      bool Buffer::is_alive() const
      {
        return m_Data.m_Type == Buffer::TYPE_ID &&
               check_alive(ms_Slots, Buffer::get_capacity());
      }

      uint32_t Buffer::get_capacity()
      {
        return ms_Capacity;
      }

      Buffer Buffer::find_by_name(Low::Util::Name p_Name)
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
      }

      Buffer Buffer::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        Buffer l_Handle = make(p_Name);
        l_Handle.set_buffer(get_buffer());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      Buffer Buffer::duplicate(Buffer p_Handle,
                               Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle Buffer::_duplicate(Low::Util::Handle p_Handle,
                                           Low::Util::Name p_Name)
      {
        Buffer l_Buffer = p_Handle.get_id();
        return l_Buffer.duplicate(p_Name);
      }

      void Buffer::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Buffer::serialize(Low::Util::Handle p_Handle,
                             Low::Util::Yaml::Node &p_Node)
      {
        Buffer l_Buffer = p_Handle.get_id();
        l_Buffer.serialize(p_Node);
      }

      Low::Util::Handle
      Buffer::deserialize(Low::Util::Yaml::Node &p_Node,
                          Low::Util::Handle p_Creator)
      {
        Buffer l_Handle = Buffer::make(N(Buffer));

        if (p_Node["buffer"]) {
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      Backend::Buffer &Buffer::get_buffer() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_buffer
        // LOW_CODEGEN::END::CUSTOM:GETTER_buffer

        return TYPE_SOA(Buffer, buffer, Backend::Buffer);
      }
      void Buffer::set_buffer(Backend::Buffer &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_buffer
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_buffer

        // Set new value
        TYPE_SOA(Buffer, buffer, Backend::Buffer) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_buffer
        // LOW_CODEGEN::END::CUSTOM:SETTER_buffer
      }

      Low::Util::Name Buffer::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Buffer, name, Low::Util::Name);
      }
      void Buffer::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(Buffer, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      Buffer Buffer::make(Util::Name p_Name,
                          Backend::BufferCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        Buffer l_Buffer = Buffer::make(p_Name);

        Backend::callbacks().buffer_create(l_Buffer.get_buffer(),
                                           p_Params);

        return l_Buffer;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      void Buffer::set(void *p_Data)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set
        Backend::callbacks().buffer_set(get_buffer(), p_Data);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set
      }

      void Buffer::write(void *p_Data, uint32_t p_DataSize,
                         uint32_t p_Start)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_write
        Backend::callbacks().buffer_write(get_buffer(), p_Data,
                                          p_DataSize, p_Start);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_write
      }

      void Buffer::read(void *p_Data, uint32_t p_DataSize,
                        uint32_t p_Start)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_read
        Backend::callbacks().buffer_read(get_buffer(), p_Data,
                                         p_DataSize, p_Start);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_read
      }

      void Buffer::bind_vertex()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_bind_vertex
        Backend::callbacks().buffer_bind_vertex(get_buffer());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_bind_vertex
      }

      void Buffer::bind_index(uint8_t p_BindType)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_bind_index
        Backend::callbacks().buffer_bind_index(get_buffer(),
                                               p_BindType);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_bind_index
      }

      uint32_t Buffer::create_instance()
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

      void Buffer::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease =
            std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0,
                   "Could not increase capacity");

        uint8_t *l_NewBuffer = (uint8_t *)malloc(
            (l_Capacity + l_CapacityIncrease) * sizeof(BufferData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(
              &l_NewBuffer[offsetof(BufferData, buffer) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(BufferData, buffer) * (l_Capacity)],
              l_Capacity * sizeof(Backend::Buffer));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(BufferData, name) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(BufferData, name) * (l_Capacity)],
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

        LOW_LOG_DEBUG << "Auto-increased budget for Buffer from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease)
                      << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Resource
  }   // namespace Renderer
} // namespace Low
