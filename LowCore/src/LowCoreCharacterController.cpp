#include "LowCoreCharacterController.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowCorePrefabInstance.h"
// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowCoreTransform.h"
#include "LowCoreScene.h"
#include "LowCore.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      Low::Util::Set<Low::Core::Component::CharacterController>
          Low::Core::Component::CharacterController::ms_Dirty;

      static Low::Core::Physics::World
      get_physics_world(Low::Core::Entity p_Entity)
      {
        _LOW_ASSERT(p_Entity.is_alive());
        Low::Core::Region l_Region = p_Entity.get_region();
        _LOW_ASSERT(l_Region.is_alive());
        Low::Core::Scene l_Scene = l_Region.get_scene();
        _LOW_ASSERT(l_Scene.is_alive());
        Low::Core::Physics::World l_World =
            l_Scene.get_physics_world();
        _LOW_ASSERT(l_World.is_alive());
        return l_World;
      }

      static Low::Math::Vector3
      world_position_to_local(Transform p_Transform,
                              Low::Math::Vector3 p_WorldPosition)
      {
        Transform l_Parent = p_Transform.get_parent();
        if (!l_Parent.is_alive()) {
          return p_WorldPosition;
        }

        Low::Math::Vector4 l_LocalPosition =
            glm::inverse(l_Parent.get_world_matrix()) *
            Low::Math::Vector4(p_WorldPosition, 1.0f);
        return Low::Math::Vector3(l_LocalPosition);
      }

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 CharacterController::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          CharacterController::IDENTIFIER(LOW_NAME(1181529166),
                                          LOW_NAME(4024897656));
      uint32_t CharacterController::ms_Capacity = 0u;
      uint32_t CharacterController::ms_PageSize = 0u;
      Low::Util::List<CharacterController>
          CharacterController::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          CharacterController::ms_Pages;

      Low::Util::Handle
      CharacterController::_make(Low::Util::Handle p_Entity)
      {
        Low::Core::Entity l_Entity = p_Entity.get_id();
        LOW_ASSERT(l_Entity.is_alive(),
                   "Cannot create component for dead entity");
        return make(l_Entity).get_id();
      }

      CharacterController
      CharacterController::make(Low::Core::Entity p_Entity)
      {
        return make(p_Entity, 0ull);
      }

      CharacterController
      CharacterController::make(Low::Core::Entity p_Entity,
                                Low::Util::UniqueId p_UniqueId)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        CharacterController l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = CharacterController::ms_TypeId;

        ACCESSOR_TYPE_SOA(l_Handle, CharacterController, height,
                          float) = 0.0f;
        ACCESSOR_TYPE_SOA(l_Handle, CharacterController, radius,
                          float) = 0.0f;
        ACCESSOR_TYPE_SOA(l_Handle, CharacterController, skin_width,
                          float) = 0.0f;
        ACCESSOR_TYPE_SOA(l_Handle, CharacterController, slope_limit,
                          float) = 0.0f;
        ACCESSOR_TYPE_SOA(l_Handle, CharacterController, step_offset,
                          float) = 0.0f;
        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, CharacterController, capsule_controller,
            Low::Core::Physics::CapsuleController))
            Low::Core::Physics::CapsuleController();
        ACCESSOR_TYPE_SOA(l_Handle, CharacterController, initialized,
                          bool) = false;
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, CharacterController,
                                   entity, Low::Core::Entity))
            Low::Core::Entity();
        ACCESSOR_TYPE_SOA(l_Handle, CharacterController, dirty,
                          bool) = false;

        l_Handle.set_entity(p_Entity);
        p_Entity.add_component(l_Handle);

        ms_LivingInstances.push_back(l_Handle);

        if (p_UniqueId > 0ull) {
          l_Handle.set_unique_id(p_UniqueId);
        } else {
          l_Handle.set_unique_id(
              Low::Util::generate_unique_id(l_Handle.get_id()));
        }
        Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                      l_Handle.get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        l_Handle.set_center(Low::Math::Vector3(0.0f, 1.0f, 0.0f));
        l_Handle.set_height(2.0f);
        l_Handle.set_radius(0.5f);
        l_Handle.set_skin_width(0.08f);
        l_Handle.set_slope_limit(45.0f);
        l_Handle.set_step_offset(0.3f);
        l_Handle.set_velocity(Low::Math::Vector3(0.0f));
        l_Handle.set_initialized(true);

        l_Handle.mark_dirty();

        {
          Low::Core::Component::Transform l_Transform =
              p_Entity.get_transform();
          if (l_Transform.is_alive()) {
            l_Transform.observe(N(world_scale_changed),
                                l_Handle.get_id());
          }
        }
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void CharacterController::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          ms_Dirty.erase(*this);
          if (get_capsule_controller().is_alive()) {
            get_capsule_controller().destroy();
          }
          set_initialized(false);
          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

        Low::Util::remove_unique_id(get_unique_id());

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

      void CharacterController::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(
            N(LowCore), N(CharacterController));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(
            N(LowCore), N(CharacterController));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, CharacterController::Data::get_size(),
                ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(CharacterController);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &CharacterController::is_alive;
        l_TypeInfo.destroy = &CharacterController::destroy;
        l_TypeInfo.serialize = &CharacterController::serialize;
        l_TypeInfo.deserialize = &CharacterController::deserialize;
        l_TypeInfo.find_by_index =
            &CharacterController::_find_by_index;
        l_TypeInfo.notify = &CharacterController::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.make_default = nullptr;
        l_TypeInfo.make_component = &CharacterController::_make;
        l_TypeInfo.duplicate_default = nullptr;
        l_TypeInfo.duplicate_component =
            &CharacterController::_duplicate;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &CharacterController::living_instances);
        l_TypeInfo.get_living_count =
            &CharacterController::living_count;
        l_TypeInfo.component = true;
        l_TypeInfo.uiComponent = false;
        {
          // Property: center
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(center);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(CharacterController::Data, center);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.get_center();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, CharacterController, center,
                Low::Math::Vector3);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.set_center(*(Low::Math::Vector3 *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CharacterController l_Handle = p_Handle.get_id();
            *((Low::Math::Vector3 *)p_Data) = l_Handle.get_center();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: center
        }
        {
          // Property: height
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(height);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(CharacterController::Data, height);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.get_height();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, CharacterController, height, float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.set_height(*(float *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CharacterController l_Handle = p_Handle.get_id();
            *((float *)p_Data) = l_Handle.get_height();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: height
        }
        {
          // Property: radius
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(radius);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(CharacterController::Data, radius);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.get_radius();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, CharacterController, radius, float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.set_radius(*(float *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CharacterController l_Handle = p_Handle.get_id();
            *((float *)p_Data) = l_Handle.get_radius();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: radius
        }
        {
          // Property: skin_width
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(skin_width);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(CharacterController::Data, skin_width);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.get_skin_width();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, CharacterController, skin_width, float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.set_skin_width(*(float *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CharacterController l_Handle = p_Handle.get_id();
            *((float *)p_Data) = l_Handle.get_skin_width();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: skin_width
        }
        {
          // Property: slope_limit
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(slope_limit);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(CharacterController::Data, slope_limit);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.get_slope_limit();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, CharacterController, slope_limit, float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.set_slope_limit(*(float *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CharacterController l_Handle = p_Handle.get_id();
            *((float *)p_Data) = l_Handle.get_slope_limit();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: slope_limit
        }
        {
          // Property: step_offset
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(step_offset);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(CharacterController::Data, step_offset);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.get_step_offset();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, CharacterController, step_offset, float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.set_step_offset(*(float *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CharacterController l_Handle = p_Handle.get_id();
            *((float *)p_Data) = l_Handle.get_step_offset();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: step_offset
        }
        {
          // Property: velocity
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(velocity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(CharacterController::Data, velocity);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.get_velocity();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, CharacterController, velocity,
                Low::Math::Vector3);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.set_velocity(*(Low::Math::Vector3 *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CharacterController l_Handle = p_Handle.get_id();
            *((Low::Math::Vector3 *)p_Data) = l_Handle.get_velocity();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: velocity
        }
        {
          // Property: capsule_controller
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(capsule_controller);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(CharacterController::Data, capsule_controller);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Low::Core::Physics::CapsuleController::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.get_capsule_controller();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, CharacterController, capsule_controller,
                Low::Core::Physics::CapsuleController);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CharacterController l_Handle = p_Handle.get_id();
            *((Low::Core::Physics::CapsuleController *)p_Data) =
                l_Handle.get_capsule_controller();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: capsule_controller
        }
        {
          // Property: initialized
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(initialized);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(CharacterController::Data, initialized);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.is_initialized();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, CharacterController, initialized, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CharacterController l_Handle = p_Handle.get_id();
            *((bool *)p_Data) = l_Handle.is_initialized();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: initialized
        }
        {
          // Property: entity
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(entity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(CharacterController::Data, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.get_entity();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, CharacterController, entity,
                Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.set_entity(*(Low::Core::Entity *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CharacterController l_Handle = p_Handle.get_id();
            *((Low::Core::Entity *)p_Data) = l_Handle.get_entity();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: entity
        }
        {
          // Property: unique_id
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(CharacterController::Data, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, CharacterController, unique_id,
                Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CharacterController l_Handle = p_Handle.get_id();
            *((Low::Util::UniqueId *)p_Data) =
                l_Handle.get_unique_id();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: unique_id
        }
        {
          // Property: dirty
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(dirty);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(CharacterController::Data, dirty);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.is_dirty();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, CharacterController, dirty, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            CharacterController l_Handle = p_Handle.get_id();
            l_Handle.set_dirty(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CharacterController l_Handle = p_Handle.get_id();
            *((bool *)p_Data) = l_Handle.is_dirty();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: dirty
        }
        {
          // Virtual property: grounded
          Low::Util::RTTI::VirtualPropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(grounded);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            CharacterController l_Handle = p_Handle.get_id();
            bool l_Data = l_Handle.is_grounded();
            memcpy(p_Data, &l_Data, sizeof(bool));
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.virtualProperties[l_PropertyInfo.name] =
              l_PropertyInfo;
          // End virtual property: grounded
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
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: move
        }
        {
          // Function: teleport
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(teleport);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Position);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR3;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: teleport
        }
        {
          // Function: rebuild
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(rebuild);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: rebuild
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
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void CharacterController::cleanup()
      {
        Low::Util::List<CharacterController> l_Instances =
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
      CharacterController::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      CharacterController
      CharacterController::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        CharacterController l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = CharacterController::ms_TypeId;

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

      CharacterController
      CharacterController::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        CharacterController l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = CharacterController::ms_TypeId;

        return l_Handle;
      }

      bool CharacterController::is_alive() const
      {
        if (m_Data.m_Type != CharacterController::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == CharacterController::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t CharacterController::get_capacity()
      {
        return ms_Capacity;
      }

      CharacterController
      CharacterController::duplicate(Low::Core::Entity p_Entity) const
      {
        _LOW_ASSERT(is_alive());

        CharacterController l_Handle = make(p_Entity);
        l_Handle.set_center(get_center());
        l_Handle.set_height(get_height());
        l_Handle.set_radius(get_radius());
        l_Handle.set_skin_width(get_skin_width());
        l_Handle.set_slope_limit(get_slope_limit());
        l_Handle.set_step_offset(get_step_offset());
        l_Handle.set_velocity(get_velocity());
        if (get_capsule_controller().is_alive()) {
          l_Handle.set_capsule_controller(get_capsule_controller());
        }
        l_Handle.set_initialized(is_initialized());
        l_Handle.set_dirty(is_dirty());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      CharacterController
      CharacterController::duplicate(CharacterController p_Handle,
                                     Low::Core::Entity p_Entity)
      {
        return p_Handle.duplicate(p_Entity);
      }

      Low::Util::Handle
      CharacterController::_duplicate(Low::Util::Handle p_Handle,
                                      Low::Util::Handle p_Entity)
      {
        CharacterController l_CharacterController = p_Handle.get_id();
        Low::Core::Entity l_Entity = p_Entity.get_id();
        return l_CharacterController.duplicate(l_Entity);
      }

      void CharacterController::serialize(
          Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["center"] = get_center();
        p_Node["height"] = get_height();
        p_Node["radius"] = get_radius();
        p_Node["skin_width"] = get_skin_width();
        p_Node["slope_limit"] = get_slope_limit();
        p_Node["step_offset"] = get_step_offset();
        p_Node["_unique_id"] = Low::Util::U64Id{get_unique_id()};

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void
      CharacterController::serialize(Low::Util::Handle p_Handle,
                                     Low::Util::Serial::Node &p_Node)
      {
        CharacterController l_CharacterController = p_Handle.get_id();
        l_CharacterController.serialize(p_Node);
      }

      Low::Util::Handle CharacterController::deserialize(
          Low::Util::Serial::Node &p_Node,
          Low::Util::Handle p_Creator)
      {
        Low::Util::UniqueId l_HandleUniqueId = 0ull;
        if (p_Node["unique_id"]) {
          l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
        } else if (p_Node["_unique_id"]) {
          l_HandleUniqueId = Low::Util::string_to_hash(
              p_Node["_unique_id"].as<Low::Util::String>());
        }

        CharacterController l_Handle = CharacterController::make(
            p_Creator.get_id(), l_HandleUniqueId);

        if (p_Node["center"]) {
          l_Handle.set_center(
              p_Node["center"].as<Low::Math::Vector3>());
        }
        if (p_Node["height"]) {
          l_Handle.set_height(p_Node["height"].as<float>());
        }
        if (p_Node["radius"]) {
          l_Handle.set_radius(p_Node["radius"].as<float>());
        }
        if (p_Node["skin_width"]) {
          l_Handle.set_skin_width(p_Node["skin_width"].as<float>());
        }
        if (p_Node["slope_limit"]) {
          l_Handle.set_slope_limit(p_Node["slope_limit"].as<float>());
        }
        if (p_Node["step_offset"]) {
          l_Handle.set_step_offset(p_Node["step_offset"].as<float>());
        }
        if (p_Node["unique_id"]) {
          l_Handle.set_unique_id(
              p_Node["unique_id"].as<Low::Util::UniqueId>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void CharacterController::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 CharacterController::observe(
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
      CharacterController::observe(Low::Util::Name p_Observable,
                                   Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void CharacterController::notify(Low::Util::Handle p_Observed,
                                       Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        if (p_Observable == N(world_scale_changed)) {
          mark_dirty();
        }
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void CharacterController::_notify(Low::Util::Handle p_Observer,
                                        Low::Util::Handle p_Observed,
                                        Low::Util::Name p_Observable)
      {
        CharacterController l_CharacterController =
            p_Observer.get_id();
        l_CharacterController.notify(p_Observed, p_Observable);
      }

      Low::Math::Vector3 CharacterController::get_center() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_center
        // LOW_CODEGEN::END::CUSTOM:GETTER_center

        return TYPE_SOA(CharacterController, center,
                        Low::Math::Vector3);
      }
      void CharacterController::set_center(float p_X, float p_Y,
                                           float p_Z)
      {
        Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
        set_center(p_Val);
      }

      void CharacterController::set_center_x(float p_Value)
      {
        Low::Math::Vector3 l_Value = get_center();
        l_Value.x = p_Value;
        set_center(l_Value);
      }

      void CharacterController::set_center_y(float p_Value)
      {
        Low::Math::Vector3 l_Value = get_center();
        l_Value.y = p_Value;
        set_center(l_Value);
      }

      void CharacterController::set_center_z(float p_Value)
      {
        Low::Math::Vector3 l_Value = get_center();
        l_Value.z = p_Value;
        set_center(l_Value);
      }

      void CharacterController::set_center(Low::Math::Vector3 p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_center
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_center

        if (get_center() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(CharacterController, center, Low::Math::Vector3) =
              p_Value;
          {
            Low::Core::Entity l_Entity = get_entity();
            if (l_Entity.has_component(
                    Low::Core::Component::PrefabInstance::
                        type_id())) {
              Low::Core::Component::PrefabInstance l_Instance =
                  l_Entity.get_component(
                      Low::Core::Component::PrefabInstance::
                          type_id());
              Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
              if (l_Prefab.is_alive()) {
                l_Instance.override(
                    ms_TypeId, N(center),
                    !l_Prefab.compare_property(*this, N(center)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_center
          // LOW_CODEGEN::END::CUSTOM:SETTER_center

          broadcast_observable(N(center));
        }
      }

      float CharacterController::get_height() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_height
        // LOW_CODEGEN::END::CUSTOM:GETTER_height

        return TYPE_SOA(CharacterController, height, float);
      }
      void CharacterController::set_height(float p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_height
        p_Value = LOW_MATH_MAX(p_Value, 0.001f);
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_height

        if (get_height() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(CharacterController, height, float) = p_Value;
          {
            Low::Core::Entity l_Entity = get_entity();
            if (l_Entity.has_component(
                    Low::Core::Component::PrefabInstance::
                        type_id())) {
              Low::Core::Component::PrefabInstance l_Instance =
                  l_Entity.get_component(
                      Low::Core::Component::PrefabInstance::
                          type_id());
              Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
              if (l_Prefab.is_alive()) {
                l_Instance.override(
                    ms_TypeId, N(height),
                    !l_Prefab.compare_property(*this, N(height)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_height
          // LOW_CODEGEN::END::CUSTOM:SETTER_height

          broadcast_observable(N(height));
        }
      }

      float CharacterController::get_radius() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_radius
        // LOW_CODEGEN::END::CUSTOM:GETTER_radius

        return TYPE_SOA(CharacterController, radius, float);
      }
      void CharacterController::set_radius(float p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_radius
        p_Value = LOW_MATH_MAX(p_Value, 0.001f);
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_radius

        if (get_radius() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(CharacterController, radius, float) = p_Value;
          {
            Low::Core::Entity l_Entity = get_entity();
            if (l_Entity.has_component(
                    Low::Core::Component::PrefabInstance::
                        type_id())) {
              Low::Core::Component::PrefabInstance l_Instance =
                  l_Entity.get_component(
                      Low::Core::Component::PrefabInstance::
                          type_id());
              Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
              if (l_Prefab.is_alive()) {
                l_Instance.override(
                    ms_TypeId, N(radius),
                    !l_Prefab.compare_property(*this, N(radius)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_radius
          // LOW_CODEGEN::END::CUSTOM:SETTER_radius

          broadcast_observable(N(radius));
        }
      }

      float CharacterController::get_skin_width() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_skin_width
        // LOW_CODEGEN::END::CUSTOM:GETTER_skin_width

        return TYPE_SOA(CharacterController, skin_width, float);
      }
      void CharacterController::set_skin_width(float p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_skin_width
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_skin_width

        if (get_skin_width() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(CharacterController, skin_width, float) = p_Value;
          {
            Low::Core::Entity l_Entity = get_entity();
            if (l_Entity.has_component(
                    Low::Core::Component::PrefabInstance::
                        type_id())) {
              Low::Core::Component::PrefabInstance l_Instance =
                  l_Entity.get_component(
                      Low::Core::Component::PrefabInstance::
                          type_id());
              Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
              if (l_Prefab.is_alive()) {
                l_Instance.override(
                    ms_TypeId, N(skin_width),
                    !l_Prefab.compare_property(*this, N(skin_width)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_skin_width
          // LOW_CODEGEN::END::CUSTOM:SETTER_skin_width

          broadcast_observable(N(skin_width));
        }
      }

      float CharacterController::get_slope_limit() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_slope_limit
        // LOW_CODEGEN::END::CUSTOM:GETTER_slope_limit

        return TYPE_SOA(CharacterController, slope_limit, float);
      }
      void CharacterController::set_slope_limit(float p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_slope_limit
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_slope_limit

        if (get_slope_limit() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(CharacterController, slope_limit, float) = p_Value;
          {
            Low::Core::Entity l_Entity = get_entity();
            if (l_Entity.has_component(
                    Low::Core::Component::PrefabInstance::
                        type_id())) {
              Low::Core::Component::PrefabInstance l_Instance =
                  l_Entity.get_component(
                      Low::Core::Component::PrefabInstance::
                          type_id());
              Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
              if (l_Prefab.is_alive()) {
                l_Instance.override(ms_TypeId, N(slope_limit),
                                    !l_Prefab.compare_property(
                                        *this, N(slope_limit)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_slope_limit
          // LOW_CODEGEN::END::CUSTOM:SETTER_slope_limit

          broadcast_observable(N(slope_limit));
        }
      }

      float CharacterController::get_step_offset() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_step_offset
        // LOW_CODEGEN::END::CUSTOM:GETTER_step_offset

        return TYPE_SOA(CharacterController, step_offset, float);
      }
      void CharacterController::set_step_offset(float p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_step_offset
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_step_offset

        if (get_step_offset() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(CharacterController, step_offset, float) = p_Value;
          {
            Low::Core::Entity l_Entity = get_entity();
            if (l_Entity.has_component(
                    Low::Core::Component::PrefabInstance::
                        type_id())) {
              Low::Core::Component::PrefabInstance l_Instance =
                  l_Entity.get_component(
                      Low::Core::Component::PrefabInstance::
                          type_id());
              Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
              if (l_Prefab.is_alive()) {
                l_Instance.override(ms_TypeId, N(step_offset),
                                    !l_Prefab.compare_property(
                                        *this, N(step_offset)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_step_offset
          // LOW_CODEGEN::END::CUSTOM:SETTER_step_offset

          broadcast_observable(N(step_offset));
        }
      }

      Low::Math::Vector3 CharacterController::get_velocity() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_velocity
        // LOW_CODEGEN::END::CUSTOM:GETTER_velocity

        return TYPE_SOA(CharacterController, velocity,
                        Low::Math::Vector3);
      }
      void CharacterController::set_velocity(float p_X, float p_Y,
                                             float p_Z)
      {
        Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
        set_velocity(p_Val);
      }

      void CharacterController::set_velocity_x(float p_Value)
      {
        Low::Math::Vector3 l_Value = get_velocity();
        l_Value.x = p_Value;
        set_velocity(l_Value);
      }

      void CharacterController::set_velocity_y(float p_Value)
      {
        Low::Math::Vector3 l_Value = get_velocity();
        l_Value.y = p_Value;
        set_velocity(l_Value);
      }

      void CharacterController::set_velocity_z(float p_Value)
      {
        Low::Math::Vector3 l_Value = get_velocity();
        l_Value.z = p_Value;
        set_velocity(l_Value);
      }

      void
      CharacterController::set_velocity(Low::Math::Vector3 p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_velocity
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_velocity

        // Set new value
        TYPE_SOA(CharacterController, velocity, Low::Math::Vector3) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_velocity
        // LOW_CODEGEN::END::CUSTOM:SETTER_velocity

        broadcast_observable(N(velocity));
      }

      Low::Core::Physics::CapsuleController
      CharacterController::get_capsule_controller() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_capsule_controller
        // LOW_CODEGEN::END::CUSTOM:GETTER_capsule_controller

        return TYPE_SOA(CharacterController, capsule_controller,
                        Low::Core::Physics::CapsuleController);
      }
      void CharacterController::set_capsule_controller(
          Low::Core::Physics::CapsuleController p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_capsule_controller
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_capsule_controller

        // Set new value
        TYPE_SOA(CharacterController, capsule_controller,
                 Low::Core::Physics::CapsuleController) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_capsule_controller
        // LOW_CODEGEN::END::CUSTOM:SETTER_capsule_controller

        broadcast_observable(N(capsule_controller));
      }

      bool CharacterController::is_initialized() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_initialized
        // LOW_CODEGEN::END::CUSTOM:GETTER_initialized

        return TYPE_SOA(CharacterController, initialized, bool);
      }
      void CharacterController::toggle_initialized()
      {
        set_initialized(!is_initialized());
      }

      void CharacterController::set_initialized(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_initialized
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_initialized

        // Set new value
        TYPE_SOA(CharacterController, initialized, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_initialized
        // LOW_CODEGEN::END::CUSTOM:SETTER_initialized

        broadcast_observable(N(initialized));
      }

      Low::Core::Entity CharacterController::get_entity() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity
        // LOW_CODEGEN::END::CUSTOM:GETTER_entity

        return TYPE_SOA(CharacterController, entity,
                        Low::Core::Entity);
      }
      void CharacterController::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entity
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_entity

        // Set new value
        TYPE_SOA(CharacterController, entity, Low::Core::Entity) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity
        // LOW_CODEGEN::END::CUSTOM:SETTER_entity

        broadcast_observable(N(entity));
      }

      Low::Util::UniqueId CharacterController::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(CharacterController, unique_id,
                        Low::Util::UniqueId);
      }
      void
      CharacterController::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(CharacterController, unique_id,
                 Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

        broadcast_observable(N(unique_id));
      }

      bool CharacterController::is_dirty() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

        return TYPE_SOA(CharacterController, dirty, bool);
      }
      void CharacterController::toggle_dirty()
      {
        set_dirty(!is_dirty());
      }

      void CharacterController::set_dirty(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

        // Set new value
        TYPE_SOA(CharacterController, dirty, bool) = p_Value;

        if (p_Value) {
          mark_dirty();
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:SETTER_dirty

        broadcast_observable(N(dirty));
      }

      void CharacterController::mark_dirty()
      {
        if (!is_dirty()) {
          TYPE_SOA(CharacterController, dirty, bool) = true;
          // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
          ms_Dirty.insert(get_id());
          // LOW_CODEGEN::END::CUSTOM:MARK_dirty
        }
      }

      void CharacterController::move(Low::Math::Vector3 p_Delta)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_move
        _LOW_ASSERT(is_alive());

        if (!get_capsule_controller().is_alive()) {
          return;
        }
        get_capsule_controller().move(p_Delta, LOW_DELTA_TIME);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_move
      }

      void
      CharacterController::teleport(Low::Math::Vector3 p_Position)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_teleport
        _LOW_ASSERT(is_alive());

        Entity l_Entity = get_entity();
        if (!l_Entity.is_alive()) {
          return;
        }
        Transform l_Transform = l_Entity.get_transform();
        if (!l_Transform.is_alive()) {
          return;
        }

        l_Transform.position(
            world_position_to_local(l_Transform, p_Position));

        if (get_capsule_controller().is_alive()) {
          Low::Math::Vector3 l_PhysicsPosition =
              p_Position +
              (l_Transform.get_world_rotation() * get_center());
          get_capsule_controller().set_position(l_PhysicsPosition);
        }
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_teleport
      }

      void CharacterController::rebuild()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_rebuild
        _LOW_ASSERT(is_alive());

        if (get_capsule_controller().is_alive()) {
          get_capsule_controller().destroy();
          set_capsule_controller(
              Low::Core::Physics::CapsuleController());
        }

        Entity l_Entity = get_entity();
        if (!l_Entity.is_alive() ||
            !l_Entity.get_transform().is_alive()) {
          set_initialized(false);
          set_velocity(Low::Math::Vector3(0.0f));
          return;
        }

        Low::Core::Physics::World l_World =
            get_physics_world(l_Entity);

        Transform l_Transform = l_Entity.get_transform();
        Low::Math::Vector3 l_Position =
            l_Transform.get_world_position() +
            (l_Transform.get_world_rotation() * get_center());
        Low::Math::Quaternion l_Rotation =
            l_Transform.get_world_rotation();

        Low::Core::Physics::CapsuleController l_CapsuleController =
            Low::Core::Physics::CapsuleController::make(
                l_World, l_Position, l_Rotation, get_height(),
                get_radius(), get_slope_limit(), get_step_offset(),
                get_skin_width());

        set_capsule_controller(l_CapsuleController);
        set_initialized(true);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_rebuild
      }

      bool CharacterController::is_grounded()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_grounded
        if (!get_capsule_controller().is_alive()) {
          return false;
        }
        return get_capsule_controller().is_grounded();
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_grounded
      }

      uint32_t CharacterController::create_instance(u32 &p_PageIndex,
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

      u32 CharacterController::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT(
            (l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
            "Could not increase capacity for CharacterController.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, CharacterController::Data::get_size(),
            ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool CharacterController::get_page_for_index(const u32 p_Index,
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

    } // namespace Component
  } // namespace Core
} // namespace Low
