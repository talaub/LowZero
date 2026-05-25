#include "LowRendererSkinningPose.h"

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
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    Low::Util::Set<Low::Renderer::SkinningPose>
        Low::Renderer::SkinningPose::ms_Dirty;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    u16 SkinningPose::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        SkinningPose::IDENTIFIER(LOW_NAME(509652687),
                                 LOW_NAME(2306435318));
    uint32_t SkinningPose::ms_Capacity = 0u;
    uint32_t SkinningPose::ms_PageSize = 0u;
    Low::Util::List<SkinningPose> SkinningPose::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        SkinningPose::ms_Pages;

    Low::Util::Handle SkinningPose::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    SkinningPose SkinningPose::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

      SkinningPose l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = SkinningPose::ms_TypeId;

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, SkinningPose, matrices,
                                 Util::List<Math::Matrix4x4>))
          Util::List<Math::Matrix4x4>();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, SkinningPose, skeleton,
                                 Skeleton)) Skeleton();
      ACCESSOR_TYPE_SOA(l_Handle, SkinningPose, uploaded, bool) =
          false;
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, SkinningPose, references,
                                 Low::Util::Set<u64>))
          Low::Util::Set<u64>();
      ACCESSOR_TYPE_SOA(l_Handle, SkinningPose, dirty, bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, SkinningPose, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void SkinningPose::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      _LOW_ASSERT(
          get_page_for_index(get_index(), l_PageIndex, l_SlotIndex));
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

    void SkinningPose::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(
          N(LowRenderer2), N(SkinningPose));

      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(SkinningPose));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, SkinningPose::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(SkinningPose);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &SkinningPose::is_alive;
      l_TypeInfo.destroy = &SkinningPose::destroy;
      l_TypeInfo.serialize = &SkinningPose::serialize;
      l_TypeInfo.deserialize = &SkinningPose::deserialize;
      l_TypeInfo.find_by_index = &SkinningPose::_find_by_index;
      l_TypeInfo.notify = &SkinningPose::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name = &SkinningPose::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &SkinningPose::_make;
      l_TypeInfo.duplicate_default = &SkinningPose::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &SkinningPose::living_instances);
      l_TypeInfo.get_living_count = &SkinningPose::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: matrices
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(matrices);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkinningPose::Data, matrices);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkinningPose l_Handle = p_Handle.get_id();
          l_Handle.get_matrices();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SkinningPose, matrices,
              Util::List<Math::Matrix4x4>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkinningPose l_Handle = p_Handle.get_id();
          *((Util::List<Math::Matrix4x4> *)p_Data) =
              l_Handle.get_matrices();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: matrices
      }
      {
        // Property: skeleton
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(skeleton);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkinningPose::Data, skeleton);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Skeleton::IDENTIFIER;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkinningPose l_Handle = p_Handle.get_id();
          l_Handle.get_skeleton();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkinningPose,
                                            skeleton, Skeleton);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkinningPose l_Handle = p_Handle.get_id();
          l_Handle.set_skeleton(*(Skeleton *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkinningPose l_Handle = p_Handle.get_id();
          *((Skeleton *)p_Data) = l_Handle.get_skeleton();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: skeleton
      }
      {
        // Property: pose_palette_offset
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(pose_palette_offset);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkinningPose::Data, pose_palette_offset);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkinningPose l_Handle = p_Handle.get_id();
          l_Handle.get_pose_palette_offset();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SkinningPose, pose_palette_offset, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkinningPose l_Handle = p_Handle.get_id();
          l_Handle.set_pose_palette_offset(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkinningPose l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_pose_palette_offset();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: pose_palette_offset
      }
      {
        // Property: uploaded
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(uploaded);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkinningPose::Data, uploaded);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkinningPose l_Handle = p_Handle.get_id();
          l_Handle.is_uploaded();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkinningPose,
                                            uploaded, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkinningPose l_Handle = p_Handle.get_id();
          l_Handle.set_uploaded(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkinningPose l_Handle = p_Handle.get_id();
          *((bool *)p_Data) = l_Handle.is_uploaded();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: uploaded
      }
      {
        // Property: references
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(references);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkinningPose::Data, references);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          return nullptr;
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: references
      }
      {
        // Property: dirty
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dirty);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkinningPose::Data, dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkinningPose l_Handle = p_Handle.get_id();
          l_Handle.is_dirty();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkinningPose,
                                            dirty, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkinningPose l_Handle = p_Handle.get_id();
          l_Handle.set_dirty(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkinningPose l_Handle = p_Handle.get_id();
          *((bool *)p_Data) = l_Handle.is_dirty();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: dirty
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkinningPose::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkinningPose l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkinningPose,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkinningPose l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkinningPose l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                        l_TypeInfo);
      // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
    }

    void SkinningPose::cleanup()
    {
      Low::Util::List<SkinningPose> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle SkinningPose::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    SkinningPose SkinningPose::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      SkinningPose l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = SkinningPose::ms_TypeId;

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

    SkinningPose SkinningPose::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      SkinningPose l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = SkinningPose::ms_TypeId;

      return l_Handle;
    }

    bool SkinningPose::is_alive() const
    {
      if (m_Data.m_Type != SkinningPose::ms_TypeId) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      return m_Data.m_Type == SkinningPose::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t SkinningPose::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    SkinningPose::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    SkinningPose SkinningPose::find_by_name(Low::Util::Name p_Name)
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

    SkinningPose SkinningPose::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      SkinningPose l_Handle = make(p_Name);
      if (get_skeleton().is_alive()) {
        l_Handle.set_skeleton(get_skeleton());
      }
      l_Handle.set_pose_palette_offset(get_pose_palette_offset());
      l_Handle.set_uploaded(is_uploaded());
      l_Handle.set_dirty(is_dirty());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    SkinningPose SkinningPose::duplicate(SkinningPose p_Handle,
                                         Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    SkinningPose::_duplicate(Low::Util::Handle p_Handle,
                             Low::Util::Name p_Name)
    {
      SkinningPose l_SkinningPose = p_Handle.get_id();
      return l_SkinningPose.duplicate(p_Name);
    }

    void
    SkinningPose::serialize(Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void SkinningPose::serialize(Low::Util::Handle p_Handle,
                                 Low::Util::Serial::Node &p_Node)
    {
      SkinningPose l_SkinningPose = p_Handle.get_id();
      l_SkinningPose.serialize(p_Node);
    }

    Low::Util::Handle
    SkinningPose::deserialize(Low::Util::Serial::Node &p_Node,
                              Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void SkinningPose::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 SkinningPose::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 SkinningPose::observe(Low::Util::Name p_Observable,
                              Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void SkinningPose::notify(Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void SkinningPose::_notify(Low::Util::Handle p_Observer,
                               Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
    {
      SkinningPose l_SkinningPose = p_Observer.get_id();
      l_SkinningPose.notify(p_Observed, p_Observable);
    }

    void SkinningPose::reference(const u64 p_Id)
    {
      _LOW_ASSERT(is_alive());

      const u32 l_OldReferences =
          (TYPE_SOA(SkinningPose, references, Low::Util::Set<u64>))
              .size();

      (TYPE_SOA(SkinningPose, references, Low::Util::Set<u64>))
          .insert(p_Id);

      const u32 l_References =
          (TYPE_SOA(SkinningPose, references, Low::Util::Set<u64>))
              .size();

      if (l_OldReferences != l_References) {
        // LOW_CODEGEN:BEGIN:CUSTOM:NEW_REFERENCE
        mark_dirty();
        // LOW_CODEGEN::END::CUSTOM:NEW_REFERENCE
      }
    }

    void SkinningPose::dereference(const u64 p_Id)
    {
      _LOW_ASSERT(is_alive());

      const u32 l_OldReferences =
          (TYPE_SOA(SkinningPose, references, Low::Util::Set<u64>))
              .size();

      (TYPE_SOA(SkinningPose, references, Low::Util::Set<u64>))
          .erase(p_Id);

      const u32 l_References =
          (TYPE_SOA(SkinningPose, references, Low::Util::Set<u64>))
              .size();

      if (l_OldReferences != l_References) {
        // LOW_CODEGEN:BEGIN:CUSTOM:REFERENCE_REMOVED
        mark_dirty();
        // LOW_CODEGEN::END::CUSTOM:REFERENCE_REMOVED
      }
    }

    u32 SkinningPose::references() const
    {
      return get_references().size();
    }

    bool SkinningPose::is_referenced() const
    {
      return !get_references().empty();
    }

    Util::List<Math::Matrix4x4> &SkinningPose::get_matrices() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_matrices
      // LOW_CODEGEN::END::CUSTOM:GETTER_matrices

      return TYPE_SOA(SkinningPose, matrices,
                      Util::List<Math::Matrix4x4>);
    }

    Skeleton SkinningPose::get_skeleton() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_skeleton
      // LOW_CODEGEN::END::CUSTOM:GETTER_skeleton

      return TYPE_SOA(SkinningPose, skeleton, Skeleton);
    }
    void SkinningPose::set_skeleton(Skeleton p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_skeleton
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_skeleton

      // Set new value
      TYPE_SOA(SkinningPose, skeleton, Skeleton) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_skeleton
      // LOW_CODEGEN::END::CUSTOM:SETTER_skeleton

      broadcast_observable(N(skeleton));
    }

    uint32_t SkinningPose::get_pose_palette_offset() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pose_palette_offset
      // LOW_CODEGEN::END::CUSTOM:GETTER_pose_palette_offset

      return TYPE_SOA(SkinningPose, pose_palette_offset, uint32_t);
    }
    void SkinningPose::set_pose_palette_offset(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_pose_palette_offset
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_pose_palette_offset

      // Set new value
      TYPE_SOA(SkinningPose, pose_palette_offset, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_pose_palette_offset
      // LOW_CODEGEN::END::CUSTOM:SETTER_pose_palette_offset

      broadcast_observable(N(pose_palette_offset));
    }

    bool SkinningPose::is_uploaded() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded

      return TYPE_SOA(SkinningPose, uploaded, bool);
    }
    void SkinningPose::toggle_uploaded()
    {
      set_uploaded(!is_uploaded());
    }

    void SkinningPose::set_uploaded(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded

      // Set new value
      TYPE_SOA(SkinningPose, uploaded, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded

      broadcast_observable(N(uploaded));
    }

    Low::Util::Set<u64> &SkinningPose::get_references() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_references
      // LOW_CODEGEN::END::CUSTOM:GETTER_references

      return TYPE_SOA(SkinningPose, references, Low::Util::Set<u64>);
    }

    bool SkinningPose::is_dirty() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

      return TYPE_SOA(SkinningPose, dirty, bool);
    }
    void SkinningPose::toggle_dirty()
    {
      set_dirty(!is_dirty());
    }

    void SkinningPose::set_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

      // Set new value
      TYPE_SOA(SkinningPose, dirty, bool) = p_Value;

      if (p_Value) {
        mark_dirty();
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:SETTER_dirty

      broadcast_observable(N(dirty));
    }

    void SkinningPose::mark_dirty()
    {
      if (!is_dirty()) {
        TYPE_SOA(SkinningPose, dirty, bool) = true;
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
        ms_Dirty.insert(get_id());
        // LOW_CODEGEN::END::CUSTOM:MARK_dirty
      }
    }

    Low::Util::Name SkinningPose::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(SkinningPose, name, Low::Util::Name);
    }
    void SkinningPose::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(SkinningPose, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t SkinningPose::create_instance(u32 &p_PageIndex,
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
          if (!ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied) {
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

    u32 SkinningPose::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for SkinningPose.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, SkinningPose::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool SkinningPose::get_page_for_index(const u32 p_Index,
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

  } // namespace Renderer
} // namespace Low
