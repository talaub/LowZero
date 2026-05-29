#include "LowCoreAnimator.h"

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
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 Animator::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          Animator::IDENTIFIER(LOW_NAME(1181529166),
                               LOW_NAME(2580148318));
      uint32_t Animator::ms_Capacity = 0u;
      uint32_t Animator::ms_PageSize = 0u;
      Low::Util::List<Animator> Animator::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          Animator::ms_Pages;

      Low::Util::Handle Animator::_make(Low::Util::Handle p_Entity)
      {
        Low::Core::Entity l_Entity = p_Entity.get_id();
        LOW_ASSERT(l_Entity.is_alive(),
                   "Cannot create component for dead entity");
        return make(l_Entity).get_id();
      }

      Animator Animator::make(Low::Core::Entity p_Entity)
      {
        return make(p_Entity, 0ull);
      }

      Animator Animator::make(Low::Core::Entity p_Entity,
                              Low::Util::UniqueId p_UniqueId)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        Animator l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Animator::ms_TypeId;

        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, Animator, render_object,
            Low::Renderer::SkeletalRenderObject))
            Low::Renderer::SkeletalRenderObject();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Animator, pose,
                                   Low::Core::Animation::Pose))
            Low::Core::Animation::Pose();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Animator,
                                   skinning_instance,
                                   Low::Renderer::SkinningInstance))
            Low::Renderer::SkinningInstance();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Animator, active_clip,
                                   Low::Core::Animation::Clip))
            Low::Core::Animation::Clip();
        ACCESSOR_TYPE_SOA(l_Handle, Animator, animation_progress,
                          float) = 0.0f;
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Animator, skeleton,
                                   Low::Renderer::Skeleton))
            Low::Renderer::Skeleton();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Animator, entity,
                                   Low::Core::Entity))
            Low::Core::Entity();

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
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Animator::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
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

      void Animator::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(
            N(LowCore), N(Animator));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(Animator));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Animator::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Animator);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Animator::is_alive;
        l_TypeInfo.destroy = &Animator::destroy;
        l_TypeInfo.serialize = &Animator::serialize;
        l_TypeInfo.deserialize = &Animator::deserialize;
        l_TypeInfo.find_by_index = &Animator::_find_by_index;
        l_TypeInfo.notify = &Animator::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.make_default = nullptr;
        l_TypeInfo.make_component = &Animator::_make;
        l_TypeInfo.duplicate_default = nullptr;
        l_TypeInfo.duplicate_component = &Animator::_duplicate;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Animator::living_instances);
        l_TypeInfo.get_living_count = &Animator::living_count;
        l_TypeInfo.component = true;
        l_TypeInfo.uiComponent = false;
        {
          // Property: render_object
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(render_object);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Animator::Data, render_object);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Low::Renderer::SkeletalRenderObject::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Animator l_Handle = p_Handle.get_id();
            l_Handle.get_render_object();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Animator, render_object,
                Low::Renderer::SkeletalRenderObject);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Animator l_Handle = p_Handle.get_id();
            l_Handle.set_render_object(
                *(Low::Renderer::SkeletalRenderObject *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Animator l_Handle = p_Handle.get_id();
            *((Low::Renderer::SkeletalRenderObject *)p_Data) =
                l_Handle.get_render_object();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: render_object
        }
        {
          // Property: pose
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(pose);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Animator::Data, pose);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Low::Core::Animation::Pose::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Animator l_Handle = p_Handle.get_id();
            l_Handle.get_pose();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Animator, pose, Low::Core::Animation::Pose);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Animator l_Handle = p_Handle.get_id();
            l_Handle.set_pose(*(Low::Core::Animation::Pose *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Animator l_Handle = p_Handle.get_id();
            *((Low::Core::Animation::Pose *)p_Data) =
                l_Handle.get_pose();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: pose
        }
        {
          // Property: skinning_instance
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(skinning_instance);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Animator::Data, skinning_instance);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Low::Renderer::SkinningInstance::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Animator l_Handle = p_Handle.get_id();
            l_Handle.get_skinning_instance();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Animator, skinning_instance,
                Low::Renderer::SkinningInstance);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Animator l_Handle = p_Handle.get_id();
            l_Handle.set_skinning_instance(
                *(Low::Renderer::SkinningInstance *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Animator l_Handle = p_Handle.get_id();
            *((Low::Renderer::SkinningInstance *)p_Data) =
                l_Handle.get_skinning_instance();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: skinning_instance
        }
        {
          // Property: active_clip
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(active_clip);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Animator::Data, active_clip);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Low::Core::Animation::Clip::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Animator l_Handle = p_Handle.get_id();
            l_Handle.get_active_clip();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Animator, active_clip,
                Low::Core::Animation::Clip);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Animator l_Handle = p_Handle.get_id();
            l_Handle.set_active_clip(
                *(Low::Core::Animation::Clip *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Animator l_Handle = p_Handle.get_id();
            *((Low::Core::Animation::Clip *)p_Data) =
                l_Handle.get_active_clip();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: active_clip
        }
        {
          // Property: animation_progress
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(animation_progress);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Animator::Data, animation_progress);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Animator l_Handle = p_Handle.get_id();
            l_Handle.get_animation_progress();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Animator, animation_progress, float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Animator l_Handle = p_Handle.get_id();
            l_Handle.set_animation_progress(*(float *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Animator l_Handle = p_Handle.get_id();
            *((float *)p_Data) = l_Handle.get_animation_progress();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: animation_progress
        }
        {
          // Property: skeleton
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(skeleton);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Animator::Data, skeleton);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Low::Renderer::Skeleton::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Animator l_Handle = p_Handle.get_id();
            l_Handle.get_skeleton();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Animator, skeleton,
                Low::Renderer::Skeleton);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Animator l_Handle = p_Handle.get_id();
            l_Handle.set_skeleton(*(Low::Renderer::Skeleton *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Animator l_Handle = p_Handle.get_id();
            *((Low::Renderer::Skeleton *)p_Data) =
                l_Handle.get_skeleton();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: skeleton
        }
        {
          // Property: entity
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(entity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Animator::Data, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Animator l_Handle = p_Handle.get_id();
            l_Handle.get_entity();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Animator, entity, Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Animator l_Handle = p_Handle.get_id();
            l_Handle.set_entity(*(Low::Core::Entity *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Animator l_Handle = p_Handle.get_id();
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
              offsetof(Animator::Data, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Animator l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Animator, unique_id, Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Animator l_Handle = p_Handle.get_id();
            *((Low::Util::UniqueId *)p_Data) =
                l_Handle.get_unique_id();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: unique_id
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void Animator::cleanup()
      {
        Low::Util::List<Animator> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle Animator::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Animator Animator::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Animator l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Animator::ms_TypeId;

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

      Animator Animator::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Animator l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Animator::ms_TypeId;

        return l_Handle;
      }

      bool Animator::is_alive() const
      {
        if (m_Data.m_Type != Animator::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == Animator::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Animator::get_capacity()
      {
        return ms_Capacity;
      }

      Animator Animator::duplicate(Low::Core::Entity p_Entity) const
      {
        _LOW_ASSERT(is_alive());

        Animator l_Handle = make(p_Entity);
        if (get_render_object().is_alive()) {
          l_Handle.set_render_object(get_render_object());
        }
        if (get_pose().is_alive()) {
          l_Handle.set_pose(get_pose());
        }
        if (get_skinning_instance().is_alive()) {
          l_Handle.set_skinning_instance(get_skinning_instance());
        }
        if (get_active_clip().is_alive()) {
          l_Handle.set_active_clip(get_active_clip());
        }
        l_Handle.set_animation_progress(get_animation_progress());
        if (get_skeleton().is_alive()) {
          l_Handle.set_skeleton(get_skeleton());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      Animator Animator::duplicate(Animator p_Handle,
                                   Low::Core::Entity p_Entity)
      {
        return p_Handle.duplicate(p_Entity);
      }

      Low::Util::Handle
      Animator::_duplicate(Low::Util::Handle p_Handle,
                           Low::Util::Handle p_Entity)
      {
        Animator l_Animator = p_Handle.get_id();
        Low::Core::Entity l_Entity = p_Entity.get_id();
        return l_Animator.duplicate(l_Entity);
      }

      void Animator::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["_unique_id"] = Low::Util::U64Id{get_unique_id()};

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Animator::serialize(Low::Util::Handle p_Handle,
                               Low::Util::Serial::Node &p_Node)
      {
        Animator l_Animator = p_Handle.get_id();
        l_Animator.serialize(p_Node);
      }

      Low::Util::Handle
      Animator::deserialize(Low::Util::Serial::Node &p_Node,
                            Low::Util::Handle p_Creator)
      {
        Low::Util::UniqueId l_HandleUniqueId = 0ull;
        if (p_Node["unique_id"]) {
          l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
        } else if (p_Node["_unique_id"]) {
          l_HandleUniqueId = Low::Util::string_to_hash(
              p_Node["_unique_id"].as<Low::Util::String>());
        }

        Animator l_Handle =
            Animator::make(p_Creator.get_id(), l_HandleUniqueId);

        if (p_Node["unique_id"]) {
          l_Handle.set_unique_id(
              p_Node["unique_id"].as<Low::Util::UniqueId>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void Animator::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64
      Animator::observe(Low::Util::Name p_Observable,
                        Low::Util::Function<void(Low::Util::Handle,
                                                 Low::Util::Name)>
                            p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Animator::observe(Low::Util::Name p_Observable,
                            Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Animator::notify(Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Animator::_notify(Low::Util::Handle p_Observer,
                             Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
      {
        Animator l_Animator = p_Observer.get_id();
        l_Animator.notify(p_Observed, p_Observable);
      }

      Low::Renderer::SkeletalRenderObject
      Animator::get_render_object() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_object
        // LOW_CODEGEN::END::CUSTOM:GETTER_render_object

        return TYPE_SOA(Animator, render_object,
                        Low::Renderer::SkeletalRenderObject);
      }
      void Animator::set_render_object(
          Low::Renderer::SkeletalRenderObject p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_object
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_object

        // Set new value
        TYPE_SOA(Animator, render_object,
                 Low::Renderer::SkeletalRenderObject) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_object
        // LOW_CODEGEN::END::CUSTOM:SETTER_render_object

        broadcast_observable(N(render_object));
      }

      Low::Core::Animation::Pose Animator::get_pose() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pose
        // LOW_CODEGEN::END::CUSTOM:GETTER_pose

        return TYPE_SOA(Animator, pose, Low::Core::Animation::Pose);
      }
      void Animator::set_pose(Low::Core::Animation::Pose p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_pose
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_pose

        // Set new value
        TYPE_SOA(Animator, pose, Low::Core::Animation::Pose) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_pose
        // LOW_CODEGEN::END::CUSTOM:SETTER_pose

        broadcast_observable(N(pose));
      }

      Low::Renderer::SkinningInstance
      Animator::get_skinning_instance() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_skinning_instance
        // LOW_CODEGEN::END::CUSTOM:GETTER_skinning_instance

        return TYPE_SOA(Animator, skinning_instance,
                        Low::Renderer::SkinningInstance);
      }
      void Animator::set_skinning_instance(
          Low::Renderer::SkinningInstance p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_skinning_instance
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_skinning_instance

        // Set new value
        TYPE_SOA(Animator, skinning_instance,
                 Low::Renderer::SkinningInstance) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_skinning_instance
        // LOW_CODEGEN::END::CUSTOM:SETTER_skinning_instance

        broadcast_observable(N(skinning_instance));
      }

      Low::Core::Animation::Clip Animator::get_active_clip() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_active_clip
        // LOW_CODEGEN::END::CUSTOM:GETTER_active_clip

        return TYPE_SOA(Animator, active_clip,
                        Low::Core::Animation::Clip);
      }
      void
      Animator::set_active_clip(Low::Core::Animation::Clip p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_active_clip
        if (p_Value == get_active_clip()) {
          return;
        }
        if (get_active_clip().is_alive()) {
          get_active_clip().dereference(get_id());
        }
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_active_clip

        // Set new value
        TYPE_SOA(Animator, active_clip, Low::Core::Animation::Clip) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_active_clip
        if (p_Value.is_alive()) {
          p_Value.reference(get_id());
        }
        // LOW_CODEGEN::END::CUSTOM:SETTER_active_clip

        broadcast_observable(N(active_clip));
      }

      float Animator::get_animation_progress() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_animation_progress
        // LOW_CODEGEN::END::CUSTOM:GETTER_animation_progress

        return TYPE_SOA(Animator, animation_progress, float);
      }
      void Animator::set_animation_progress(float p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_animation_progress
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_animation_progress

        // Set new value
        TYPE_SOA(Animator, animation_progress, float) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_animation_progress
        // LOW_CODEGEN::END::CUSTOM:SETTER_animation_progress

        broadcast_observable(N(animation_progress));
      }

      Low::Renderer::Skeleton Animator::get_skeleton() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_skeleton
        // LOW_CODEGEN::END::CUSTOM:GETTER_skeleton

        return TYPE_SOA(Animator, skeleton, Low::Renderer::Skeleton);
      }
      void Animator::set_skeleton(Low::Renderer::Skeleton p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_skeleton
        if (get_skeleton().is_alive()) {
          get_skeleton().dereference(get_id());
        }
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_skeleton

        // Set new value
        TYPE_SOA(Animator, skeleton, Low::Renderer::Skeleton) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_skeleton
        if (p_Value.is_alive()) {
          p_Value.reference(get_id());
        }
        // LOW_CODEGEN::END::CUSTOM:SETTER_skeleton

        broadcast_observable(N(skeleton));
      }

      Low::Core::Entity Animator::get_entity() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity
        // LOW_CODEGEN::END::CUSTOM:GETTER_entity

        return TYPE_SOA(Animator, entity, Low::Core::Entity);
      }
      void Animator::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entity
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_entity

        // Set new value
        TYPE_SOA(Animator, entity, Low::Core::Entity) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity
        // LOW_CODEGEN::END::CUSTOM:SETTER_entity

        broadcast_observable(N(entity));
      }

      Low::Util::UniqueId Animator::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(Animator, unique_id, Low::Util::UniqueId);
      }
      void Animator::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(Animator, unique_id, Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

        broadcast_observable(N(unique_id));
      }

      uint32_t Animator::create_instance(u32 &p_PageIndex,
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

      u32 Animator::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Animator.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Animator::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Animator::get_page_for_index(const u32 p_Index,
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
