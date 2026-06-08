#include "LowCorePhysicsBody.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowCorePhysicsBackend.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Physics {
// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
#define BACKEND_WORLD(p_World)                                       \
  static_cast<WorldBackend *>((p_World).get_world_ptr())
      static Low::Math::Matrix4x4 g_BodyTransformScratch(1.0f);
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 Body::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          Body::IDENTIFIER(LOW_NAME(1181529166),
                           LOW_NAME(2073732236));
      uint32_t Body::ms_Capacity = 0u;
      uint32_t Body::ms_PageSize = 0u;
      Low::Util::List<Body> Body::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *> Body::ms_Pages;

      Low::Util::Handle Body::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Body Body::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        Body l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Body::ms_TypeId;

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Body, world, World))
            World();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Body, shape, Shape))
            Shape();
        ACCESSOR_TYPE_SOA(l_Handle, Body, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        l_Handle.set_backend_id(0u);
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Body::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          if (get_backend_id() != 0u && get_world().is_alive()) {
            destroy_body(BACKEND_WORLD(get_world()),
                         BodyBackendHandle{get_backend_id()});
            set_backend_id(0u);
          }
          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        _LOW_ASSERT(get_page_for_index(get_index(), l_PageIndex,
                                       l_SlotIndex));
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

        l_Page->slots[l_SlotIndex].m_Occupied = false;
        l_Page->slots[l_SlotIndex].m_Generation++;

        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end();) {
          if (it->get_id() == get_id()) {
            it = ms_LivingInstances.erase(it);
          } else {
            it++;
          }
        }
      }

      void Body::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(N(LowCore),
                                                          N(Body));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(Body));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Body::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Body);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Body::is_alive;
        l_TypeInfo.destroy = &Body::destroy;
        l_TypeInfo.serialize = &Body::serialize;
        l_TypeInfo.deserialize = &Body::deserialize;
        l_TypeInfo.find_by_index = &Body::_find_by_index;
        l_TypeInfo.notify = &Body::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &Body::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Body::_make;
        l_TypeInfo.duplicate_default = &Body::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Body::living_instances);
        l_TypeInfo.get_living_count = &Body::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: backend_id
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(backend_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Body::Data, backend_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Body l_Handle = p_Handle.get_id();
            l_Handle.get_backend_id();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Body,
                                              backend_id, uint64_t);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Body l_Handle = p_Handle.get_id();
            *((uint64_t *)p_Data) = l_Handle.get_backend_id();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: backend_id
        }
        {
          // Property: world
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(world);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Body::Data, world);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = World::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Body l_Handle = p_Handle.get_id();
            l_Handle.get_world();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Body, world,
                                              World);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Body l_Handle = p_Handle.get_id();
            *((World *)p_Data) = l_Handle.get_world();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: world
        }
        {
          // Property: shape
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(shape);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Body::Data, shape);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Shape::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Body l_Handle = p_Handle.get_id();
            l_Handle.get_shape();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Body, shape,
                                              Shape);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Body l_Handle = p_Handle.get_id();
            *((Shape *)p_Data) = l_Handle.get_shape();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: shape
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Body::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Body l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Body, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Body l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Body l_Handle = p_Handle.get_id();
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Virtual property: position
          Low::Util::RTTI::VirtualPropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(position);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Body l_Handle = p_Handle.get_id();
            Low::Math::Vector3 l_Data = l_Handle.get_position();
            memcpy(p_Data, &l_Data, sizeof(Low::Math::Vector3));
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Body l_Handle = p_Handle.get_id();
            l_Handle.set_position(*(Low::Math::Vector3 *)p_Data);
          };
          l_TypeInfo.virtualProperties[l_PropertyInfo.name] =
              l_PropertyInfo;
          // End virtual property: position
        }
        {
          // Virtual property: rotation
          Low::Util::RTTI::VirtualPropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(rotation);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::QUATERNION;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Body l_Handle = p_Handle.get_id();
            Low::Math::Quaternion l_Data = l_Handle.get_rotation();
            memcpy(p_Data, &l_Data, sizeof(Low::Math::Quaternion));
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Body l_Handle = p_Handle.get_id();
            l_Handle.set_rotation(*(Low::Math::Quaternion *)p_Data);
          };
          l_TypeInfo.virtualProperties[l_PropertyInfo.name] =
              l_PropertyInfo;
          // End virtual property: rotation
        }
        {
          // Virtual property: transform
          Low::Util::RTTI::VirtualPropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(transform);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Body l_Handle = p_Handle.get_id();
            Low::Math::Matrix4x4 l_Data = l_Handle.get_transform();
            memcpy(p_Data, &l_Data, sizeof(Low::Math::Matrix4x4));
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Body l_Handle = p_Handle.get_id();
            l_Handle.set_transform(*(Low::Math::Matrix4x4 *)p_Data);
          };
          l_TypeInfo.virtualProperties[l_PropertyInfo.name] =
              l_PropertyInfo;
          // End virtual property: transform
        }
        {
          // Virtual property: linear_velocity
          Low::Util::RTTI::VirtualPropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(linear_velocity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Body l_Handle = p_Handle.get_id();
            Low::Math::Vector3 l_Data =
                l_Handle.get_linear_velocity();
            memcpy(p_Data, &l_Data, sizeof(Low::Math::Vector3));
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Body l_Handle = p_Handle.get_id();
            l_Handle.set_linear_velocity(
                *(Low::Math::Vector3 *)p_Data);
          };
          l_TypeInfo.virtualProperties[l_PropertyInfo.name] =
              l_PropertyInfo;
          // End virtual property: linear_velocity
        }
        {
          // Virtual property: angular_velocity
          Low::Util::RTTI::VirtualPropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(angular_velocity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Body l_Handle = p_Handle.get_id();
            Low::Math::Vector3 l_Data =
                l_Handle.get_angular_velocity();
            memcpy(p_Data, &l_Data, sizeof(Low::Math::Vector3));
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Body l_Handle = p_Handle.get_id();
            l_Handle.set_angular_velocity(
                *(Low::Math::Vector3 *)p_Data);
          };
          l_TypeInfo.virtualProperties[l_PropertyInfo.name] =
              l_PropertyInfo;
          // End virtual property: angular_velocity
        }
        {
          // Function: make
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(make);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType = Body::type_id();
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_World);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = World::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Shape);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Shape::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Position);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR3;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Rotation);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::QUATERNION;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_MotionType);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::ENUM;
            l_ParameterInfo.handleType =
                BodyMotionTypeEnumHelper::get_enum_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Mass);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Gravity);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::BOOL;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: make
        }
        {
          // Function: get_position
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_position);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_position
        }
        {
          // Function: set_position
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(set_position);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Value);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR3;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: set_position
        }
        {
          // Function: get_rotation
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_rotation);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::QUATERNION;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_rotation
        }
        {
          // Function: set_rotation
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(set_rotation);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Value);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::QUATERNION;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: set_rotation
        }
        {
          // Function: get_transform
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_transform);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_transform
        }
        {
          // Function: set_transform
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(set_transform);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Value);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: set_transform
        }
        {
          // Function: get_linear_velocity
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_linear_velocity);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_linear_velocity
        }
        {
          // Function: set_linear_velocity
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(set_linear_velocity);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Value);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR3;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: set_linear_velocity
        }
        {
          // Function: get_angular_velocity
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_angular_velocity);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_angular_velocity
        }
        {
          // Function: set_angular_velocity
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(set_angular_velocity);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Value);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR3;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: set_angular_velocity
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void Body::cleanup()
      {
        Low::Util::List<Body> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
          Low::Util::Instances::Page *i_Page = *it;
          free(i_Page->buffer);
          free(i_Page->slots);
          delete i_Page;
          it = ms_Pages.erase(it);
        }

        ms_Capacity = 0;
      }

      Low::Util::Handle Body::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Body Body::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Body l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Body::ms_TypeId;

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
          l_Handle.m_Data.m_Generation = 0;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        l_Handle.m_Data.m_Generation =
            l_Page->slots[l_SlotIndex].m_Generation;

        return l_Handle;
      }

      Body Body::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Body l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Body::ms_TypeId;

        return l_Handle;
      }

      bool Body::is_alive() const
      {
        if (m_Data.m_Type != Body::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == Body::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Body::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle Body::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      Body Body::find_by_name(Low::Util::Name p_Name)
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

      Body Body::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        LOW_ASSERT_WARN(false, "Not implemented");
        return 0;
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE
      }

      Body Body::duplicate(Body p_Handle, Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle Body::_duplicate(Low::Util::Handle p_Handle,
                                         Low::Util::Name p_Name)
      {
        Body l_Body = p_Handle.get_id();
        return l_Body.duplicate(p_Name);
      }

      void Body::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Body::serialize(Low::Util::Handle p_Handle,
                           Low::Util::Serial::Node &p_Node)
      {
        Body l_Body = p_Handle.get_id();
        l_Body.serialize(p_Node);
      }

      Low::Util::Handle
      Body::deserialize(Low::Util::Serial::Node &p_Node,
                        Low::Util::Handle p_Creator)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        return DEAD;
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
      }

      void
      Body::broadcast_observable(Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 Body::observe(Low::Util::Name p_Observable,
                        Low::Util::Function<void(Low::Util::Handle,
                                                 Low::Util::Name)>
                            p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Body::observe(Low::Util::Name p_Observable,
                        Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Body::notify(Low::Util::Handle p_Observed,
                        Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Body::_notify(Low::Util::Handle p_Observer,
                         Low::Util::Handle p_Observed,
                         Low::Util::Name p_Observable)
      {
        Body l_Body = p_Observer.get_id();
        l_Body.notify(p_Observed, p_Observable);
      }

      uint64_t Body::get_backend_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_backend_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_backend_id

        return TYPE_SOA(Body, backend_id, uint64_t);
      }
      void Body::set_backend_id(uint64_t p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_backend_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_backend_id

        // Set new value
        TYPE_SOA(Body, backend_id, uint64_t) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_backend_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_backend_id

        broadcast_observable(N(backend_id));
      }

      World Body::get_world() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world
        // LOW_CODEGEN::END::CUSTOM:GETTER_world

        return TYPE_SOA(Body, world, World);
      }
      void Body::set_world(World p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_world

        // Set new value
        TYPE_SOA(Body, world, World) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world
        // LOW_CODEGEN::END::CUSTOM:SETTER_world

        broadcast_observable(N(world));
      }

      Shape Body::get_shape() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_shape
        // LOW_CODEGEN::END::CUSTOM:GETTER_shape

        return TYPE_SOA(Body, shape, Shape);
      }
      void Body::set_shape(Shape p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_shape
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_shape

        // Set new value
        TYPE_SOA(Body, shape, Shape) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_shape
        // LOW_CODEGEN::END::CUSTOM:SETTER_shape

        broadcast_observable(N(shape));
      }

      Low::Util::Name Body::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Body, name, Low::Util::Name);
      }
      void Body::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(Body, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      Body Body::make(World p_World, Shape p_Shape,
                      Low::Math::Vector3 p_Position,
                      Low::Math::Quaternion p_Rotation,
                      BodyMotionType p_MotionType, float p_Mass,
                      bool p_Gravity)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        _LOW_ASSERT(p_World.is_alive());
        _LOW_ASSERT(p_Shape.is_alive());
        _LOW_ASSERT(p_Shape.get_world().get_id() == p_World.get_id());

        BodyCreateInfo l_CreateInfo;
        l_CreateInfo.shape =
            ShapeBackendHandle{p_Shape.get_backend_id()};
        l_CreateInfo.position = p_Position;
        l_CreateInfo.rotation = p_Rotation;
        l_CreateInfo.motion_type = p_MotionType;
        l_CreateInfo.mass = p_Mass;
        l_CreateInfo.gravity = p_Gravity;

        BodyBackendHandle l_BackendHandle =
            create_body(BACKEND_WORLD(p_World), l_CreateInfo);
        _LOW_ASSERT(l_BackendHandle.is_valid());

        Body l_Body = Body::make(p_World.get_name());
        l_Body.set_world(p_World);
        l_Body.set_shape(p_Shape);
        l_Body.set_backend_id(l_BackendHandle.id);

        return l_Body;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      Low::Math::Vector3 Body::get_position()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_position
        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(get_world().is_alive());

        return get_body_position(BACKEND_WORLD(get_world()),
                                 BodyBackendHandle{get_backend_id()});
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_position
      }

      void Body::set_position(Low::Math::Vector3 p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_position
        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(get_world().is_alive());

        set_body_transform(BACKEND_WORLD(get_world()),
                           BodyBackendHandle{get_backend_id()},
                           p_Value, get_rotation());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_position
      }

      Low::Math::Quaternion Body::get_rotation()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_rotation
        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(get_world().is_alive());

        return get_body_rotation(BACKEND_WORLD(get_world()),
                                 BodyBackendHandle{get_backend_id()});
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_rotation
      }

      void Body::set_rotation(Low::Math::Quaternion p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_rotation
        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(get_world().is_alive());

        set_body_transform(BACKEND_WORLD(get_world()),
                           BodyBackendHandle{get_backend_id()},
                           get_position(), p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_rotation
      }

      Low::Math::Matrix4x4 &Body::get_transform()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_transform
        _LOW_ASSERT(is_alive());

        g_BodyTransformScratch = glm::mat4_cast(get_rotation());
        g_BodyTransformScratch[3] =
            Low::Math::Vector4(get_position(), 1.0f);

        return g_BodyTransformScratch;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_transform
      }

      void Body::set_transform(Low::Math::Matrix4x4 &p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_transform
        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(get_world().is_alive());

        const Low::Math::Vector3 l_Position =
            Low::Math::Vector3(p_Value[3]);
        const Low::Math::Quaternion l_Rotation =
            glm::quat_cast(p_Value);

        set_body_transform(BACKEND_WORLD(get_world()),
                           BodyBackendHandle{get_backend_id()},
                           l_Position, l_Rotation);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_transform
      }

      Low::Math::Vector3 Body::get_linear_velocity()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_linear_velocity
        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(get_world().is_alive());

        return get_body_linear_velocity(
            BACKEND_WORLD(get_world()),
            BodyBackendHandle{get_backend_id()});
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_linear_velocity
      }

      void Body::set_linear_velocity(Low::Math::Vector3 p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_linear_velocity
        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(get_world().is_alive());

        set_body_linear_velocity(BACKEND_WORLD(get_world()),
                                 BodyBackendHandle{get_backend_id()},
                                 p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_linear_velocity
      }

      Low::Math::Vector3 Body::get_angular_velocity()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_angular_velocity
        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(get_world().is_alive());

        return get_body_angular_velocity(
            BACKEND_WORLD(get_world()),
            BodyBackendHandle{get_backend_id()});
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_angular_velocity
      }

      void Body::set_angular_velocity(Low::Math::Vector3 p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_angular_velocity
        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(get_world().is_alive());

        set_body_angular_velocity(BACKEND_WORLD(get_world()),
                                  BodyBackendHandle{get_backend_id()},
                                  p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_angular_velocity
      }

      uint32_t Body::create_instance(u32 &p_PageIndex,
                                     u32 &p_SlotIndex)
      {
        u32 l_Index = 0;
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        bool l_FoundIndex = false;

        for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
             ++l_PageIndex) {
          for (l_SlotIndex = 0;
               l_SlotIndex < ms_Pages[l_PageIndex]->size;
               ++l_SlotIndex) {
            if (!ms_Pages[l_PageIndex]
                     ->slots[l_SlotIndex]
                     .m_Occupied) {
              l_FoundIndex = true;
              break;
            }
            l_Index++;
          }
          if (l_FoundIndex) {
            break;
          }
        }
        if (!l_FoundIndex) {
          l_SlotIndex = 0;
          l_PageIndex = create_page();
        }
        ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
        p_PageIndex = l_PageIndex;
        p_SlotIndex = l_SlotIndex;
        return l_Index;
      }

      u32 Body::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Body.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Body::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Body::get_page_for_index(const u32 p_Index,
                                    u32 &p_PageIndex,
                                    u32 &p_SlotIndex)
      {
        if (p_Index >= get_capacity()) {
          p_PageIndex = LOW_UINT32_MAX;
          p_SlotIndex = LOW_UINT32_MAX;
          return false;
        }
        p_PageIndex = p_Index / ms_PageSize;
        if (p_PageIndex > (ms_Pages.size() - 1)) {
          return false;
        }
        p_SlotIndex = p_Index - (ms_PageSize * p_PageIndex);
        return true;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Physics
  } // namespace Core
} // namespace Low
