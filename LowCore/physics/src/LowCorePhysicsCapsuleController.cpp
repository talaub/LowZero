#include "LowCorePhysicsCapsuleController.h"

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
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 CapsuleController::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          CapsuleController::IDENTIFIER(LOW_NAME(1181529166),
                                        LOW_NAME(2339194545));
      uint32_t CapsuleController::ms_Capacity = 0u;
      uint32_t CapsuleController::ms_PageSize = 0u;
      Low::Util::List<CapsuleController>
          CapsuleController::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          CapsuleController::ms_Pages;

      Low::Util::Handle
      CapsuleController::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      CapsuleController
      CapsuleController::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        CapsuleController l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = CapsuleController::ms_TypeId;

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, CapsuleController, world,
                                   World)) World();
        ACCESSOR_TYPE_SOA(l_Handle, CapsuleController, name,
                          Low::Util::Name) = Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        l_Handle.set_backend_id(0u);
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void CapsuleController::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          if (get_backend_id() != 0u && get_world().is_alive()) {
            destroy_capsule_controller(
                BACKEND_WORLD(get_world()),
                CapsuleControllerBackendHandle{get_backend_id()});
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

      void CapsuleController::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(
            N(LowCore), N(CapsuleController));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(
            N(LowCore), N(CapsuleController));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, CapsuleController::Data::get_size(),
                ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(CapsuleController);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &CapsuleController::is_alive;
        l_TypeInfo.destroy = &CapsuleController::destroy;
        l_TypeInfo.serialize = &CapsuleController::serialize;
        l_TypeInfo.deserialize = &CapsuleController::deserialize;
        l_TypeInfo.find_by_index = &CapsuleController::_find_by_index;
        l_TypeInfo.notify = &CapsuleController::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &CapsuleController::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &CapsuleController::_make;
        l_TypeInfo.duplicate_default = &CapsuleController::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &CapsuleController::living_instances);
        l_TypeInfo.get_living_count =
            &CapsuleController::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: backend_id
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(backend_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(CapsuleController::Data, backend_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            CapsuleController l_Handle = p_Handle.get_id();
            l_Handle.get_backend_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, CapsuleController, backend_id, uint64_t);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CapsuleController l_Handle = p_Handle.get_id();
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
          l_PropertyInfo.dataOffset =
              offsetof(CapsuleController::Data, world);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = World::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            CapsuleController l_Handle = p_Handle.get_id();
            l_Handle.get_world();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, CapsuleController, world, World);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CapsuleController l_Handle = p_Handle.get_id();
            *((World *)p_Data) = l_Handle.get_world();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: world
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(CapsuleController::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            CapsuleController l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, CapsuleController, name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            CapsuleController l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CapsuleController l_Handle = p_Handle.get_id();
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
            CapsuleController l_Handle = p_Handle.get_id();
            Low::Math::Vector3 l_Data = l_Handle.get_position();
            memcpy(p_Data, &l_Data, sizeof(Low::Math::Vector3));
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            CapsuleController l_Handle = p_Handle.get_id();
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
            CapsuleController l_Handle = p_Handle.get_id();
            Low::Math::Quaternion l_Data = l_Handle.get_rotation();
            memcpy(p_Data, &l_Data, sizeof(Low::Math::Quaternion));
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            CapsuleController l_Handle = p_Handle.get_id();
            l_Handle.set_rotation(*(Low::Math::Quaternion *)p_Data);
          };
          l_TypeInfo.virtualProperties[l_PropertyInfo.name] =
              l_PropertyInfo;
          // End virtual property: rotation
        }
        {
          // Function: make
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(make);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType = CapsuleController::type_id();
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
            l_ParameterInfo.name = N(p_Height);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Radius);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_SlopeLimit);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_StepOffset);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_SkinWidth);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: make
        }
        {
          // Function: move
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(move);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Delta);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR3;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_DeltaTime);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: move
        }
        {
          // Function: is_grounded
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(is_grounded);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: is_grounded
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
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void CapsuleController::cleanup()
      {
        Low::Util::List<CapsuleController> l_Instances =
            ms_LivingInstances;
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

      Low::Util::Handle
      CapsuleController::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      CapsuleController
      CapsuleController::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        CapsuleController l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = CapsuleController::ms_TypeId;

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

      CapsuleController
      CapsuleController::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        CapsuleController l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = CapsuleController::ms_TypeId;

        return l_Handle;
      }

      bool CapsuleController::is_alive() const
      {
        if (m_Data.m_Type != CapsuleController::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == CapsuleController::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t CapsuleController::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      CapsuleController::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      CapsuleController
      CapsuleController::find_by_name(Low::Util::Name p_Name)
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

      CapsuleController
      CapsuleController::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        LOW_ASSERT_WARN(false, "Not implemented");
        return 0;
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE
      }

      CapsuleController
      CapsuleController::duplicate(CapsuleController p_Handle,
                                   Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      CapsuleController::_duplicate(Low::Util::Handle p_Handle,
                                    Low::Util::Name p_Name)
      {
        CapsuleController l_CapsuleController = p_Handle.get_id();
        return l_CapsuleController.duplicate(p_Name);
      }

      void CapsuleController::serialize(
          Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void
      CapsuleController::serialize(Low::Util::Handle p_Handle,
                                   Low::Util::Serial::Node &p_Node)
      {
        CapsuleController l_CapsuleController = p_Handle.get_id();
        l_CapsuleController.serialize(p_Node);
      }

      Low::Util::Handle
      CapsuleController::deserialize(Low::Util::Serial::Node &p_Node,
                                     Low::Util::Handle p_Creator)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        return Low::Util::Handle::DEAD;
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
      }

      void CapsuleController::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 CapsuleController::observe(
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

      u64
      CapsuleController::observe(Low::Util::Name p_Observable,
                                 Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void CapsuleController::notify(Low::Util::Handle p_Observed,
                                     Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void CapsuleController::_notify(Low::Util::Handle p_Observer,
                                      Low::Util::Handle p_Observed,
                                      Low::Util::Name p_Observable)
      {
        CapsuleController l_CapsuleController = p_Observer.get_id();
        l_CapsuleController.notify(p_Observed, p_Observable);
      }

      uint64_t CapsuleController::get_backend_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_backend_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_backend_id

        return TYPE_SOA(CapsuleController, backend_id, uint64_t);
      }
      void CapsuleController::set_backend_id(uint64_t p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_backend_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_backend_id

        // Set new value
        TYPE_SOA(CapsuleController, backend_id, uint64_t) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_backend_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_backend_id

        broadcast_observable(N(backend_id));
      }

      World CapsuleController::get_world() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world
        // LOW_CODEGEN::END::CUSTOM:GETTER_world

        return TYPE_SOA(CapsuleController, world, World);
      }
      void CapsuleController::set_world(World p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_world

        // Set new value
        TYPE_SOA(CapsuleController, world, World) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world
        // LOW_CODEGEN::END::CUSTOM:SETTER_world

        broadcast_observable(N(world));
      }

      Low::Util::Name CapsuleController::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(CapsuleController, name, Low::Util::Name);
      }
      void CapsuleController::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(CapsuleController, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      CapsuleController CapsuleController::make(
          World p_World, Low::Math::Vector3 p_Position,
          Low::Math::Quaternion p_Rotation, float p_Height,
          float p_Radius, float p_SlopeLimit, float p_StepOffset,
          float p_SkinWidth)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        _LOW_ASSERT(p_World.is_alive());

        CapsuleControllerCreateInfo l_CreateInfo;
        l_CreateInfo.position = p_Position;
        l_CreateInfo.rotation = p_Rotation;
        l_CreateInfo.height = p_Height;
        l_CreateInfo.radius = p_Radius;
        l_CreateInfo.slope_limit = p_SlopeLimit;
        l_CreateInfo.step_offset = p_StepOffset;
        l_CreateInfo.skin_width = p_SkinWidth;

        CapsuleControllerBackendHandle l_BackendHandle =
            create_capsule_controller(BACKEND_WORLD(p_World),
                                      l_CreateInfo);
        _LOW_ASSERT(l_BackendHandle.is_valid());

        CapsuleController l_Controller =
            CapsuleController::make(p_World.get_name());
        l_Controller.set_world(p_World);
        l_Controller.set_backend_id(l_BackendHandle.id);

        return l_Controller;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      void CapsuleController::move(Low::Math::Vector3 p_Delta,
                                   float p_DeltaTime)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_move
        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(get_world().is_alive());

        move_capsule_controller(
            BACKEND_WORLD(get_world()),
            CapsuleControllerBackendHandle{get_backend_id()}, p_Delta,
            p_DeltaTime);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_move
      }

      bool CapsuleController::is_grounded()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_grounded
        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(get_world().is_alive());

        return is_capsule_controller_grounded(
            BACKEND_WORLD(get_world()),
            CapsuleControllerBackendHandle{get_backend_id()});
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_grounded
      }

      Low::Math::Vector3 CapsuleController::get_position()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_position
        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(get_world().is_alive());

        return get_capsule_controller_position(
            BACKEND_WORLD(get_world()),
            CapsuleControllerBackendHandle{get_backend_id()});
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_position
      }

      void CapsuleController::set_position(Low::Math::Vector3 p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_position
        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(get_world().is_alive());

        set_capsule_controller_position(
            BACKEND_WORLD(get_world()),
            CapsuleControllerBackendHandle{get_backend_id()},
            p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_position
      }

      Low::Math::Quaternion CapsuleController::get_rotation()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_rotation
        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(get_world().is_alive());

        return get_capsule_controller_rotation(
            BACKEND_WORLD(get_world()),
            CapsuleControllerBackendHandle{get_backend_id()});
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_rotation
      }

      void
      CapsuleController::set_rotation(Low::Math::Quaternion p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_rotation
        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(get_world().is_alive());

        set_capsule_controller_rotation(
            BACKEND_WORLD(get_world()),
            CapsuleControllerBackendHandle{get_backend_id()},
            p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_rotation
      }

      uint32_t CapsuleController::create_instance(u32 &p_PageIndex,
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

      u32 CapsuleController::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT(
            (l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
            "Could not increase capacity for CapsuleController.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, CapsuleController::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool CapsuleController::get_page_for_index(const u32 p_Index,
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
