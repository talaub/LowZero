#include "LowRendererVkTexExport.h"

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
#include "LowRendererTextureExport.h"
#include "LowRendererVulkanBuffer.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t TexExport::TYPE_ID = 81;
      uint32_t TexExport::ms_Capacity = 0u;
      uint32_t TexExport::ms_PageSize = 0u;
      Low::Util::SharedMutex TexExport::ms_LivingMutex;
      Low::Util::SharedMutex TexExport::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          TexExport::ms_PagesLock(TexExport::ms_PagesMutex,
                                  std::defer_lock);
      Low::Util::List<TexExport> TexExport::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          TexExport::ms_Pages;

      TexExport::TexExport() : Low::Util::Handle(0ull)
      {
      }
      TexExport::TexExport(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      TexExport::TexExport(TexExport &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle TexExport::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      TexExport TexExport::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        TexExport l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = TexExport::TYPE_ID;

        l_PageLock.unlock();

        Low::Util::HandleLock<TexExport> l_HandleLock(l_Handle);

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, TexExport,
                                   staging_buffer, AllocatedBuffer))
            AllocatedBuffer();
        ACCESSOR_TYPE_SOA(l_Handle, TexExport, name,
                          Low::Util::Name) = Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        {
          Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          ms_LivingInstances.push_back(l_Handle);
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void TexExport::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<TexExport> l_Lock(get_id());
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          BufferUtil::destroy_buffer(get_staging_buffer());
          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        _LOW_ASSERT(get_page_for_index(get_index(), l_PageIndex,
                                       l_SlotIndex));
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
            l_Page->mutex);
        l_Page->slots[l_SlotIndex].m_Occupied = false;
        l_Page->slots[l_SlotIndex].m_Generation++;

        ms_PagesLock.lock();
        Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end();) {
          if (it->get_id() == get_id()) {
            it = ms_LivingInstances.erase(it);
          } else {
            it++;
          }
        }
        ms_PagesLock.unlock();
        l_LivingLock.unlock();
      }

      void TexExport::initialize()
      {
        LOCK_PAGES_WRITE(l_PagesLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                      N(TexExport));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, TexExport::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(TexExport);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &TexExport::is_alive;
        l_TypeInfo.destroy = &TexExport::destroy;
        l_TypeInfo.serialize = &TexExport::serialize;
        l_TypeInfo.deserialize = &TexExport::deserialize;
        l_TypeInfo.find_by_index = &TexExport::_find_by_index;
        l_TypeInfo.notify = &TexExport::_notify;
        l_TypeInfo.find_by_name = &TexExport::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &TexExport::_make;
        l_TypeInfo.duplicate_default = &TexExport::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &TexExport::living_instances);
        l_TypeInfo.get_living_count = &TexExport::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: staging_buffer
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(staging_buffer);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(TexExport::Data, staging_buffer);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            TexExport l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<TexExport> l_HandleLock(l_Handle);
            l_Handle.get_staging_buffer();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, TexExport, staging_buffer, AllocatedBuffer);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            TexExport l_Handle = p_Handle.get_id();
            l_Handle.set_staging_buffer(*(AllocatedBuffer *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            TexExport l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<TexExport> l_HandleLock(l_Handle);
            *((AllocatedBuffer *)p_Data) =
                l_Handle.get_staging_buffer();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: staging_buffer
        }
        {
          // Property: frame_index
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(frame_index);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(TexExport::Data, frame_index);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            TexExport l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<TexExport> l_HandleLock(l_Handle);
            l_Handle.get_frame_index();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TexExport,
                                              frame_index, uint32_t);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            TexExport l_Handle = p_Handle.get_id();
            l_Handle.set_frame_index(*(uint32_t *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            TexExport l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<TexExport> l_HandleLock(l_Handle);
            *((uint32_t *)p_Data) = l_Handle.get_frame_index();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: frame_index
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(TexExport::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            TexExport l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<TexExport> l_HandleLock(l_Handle);
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TexExport,
                                              name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            TexExport l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            TexExport l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<TexExport> l_HandleLock(l_Handle);
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void TexExport::cleanup()
      {
        Low::Util::List<TexExport> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        ms_PagesLock.lock();
        for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
          Low::Util::Instances::Page *i_Page = *it;
          free(i_Page->buffer);
          free(i_Page->slots);
          free(i_Page->lockWords);
          delete i_Page;
          it = ms_Pages.erase(it);
        }

        ms_Capacity = 0;

        ms_PagesLock.unlock();
      }

      Low::Util::Handle TexExport::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      TexExport TexExport::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        TexExport l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = TexExport::TYPE_ID;

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
          l_Handle.m_Data.m_Generation = 0;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
            l_Page->mutex);
        l_Handle.m_Data.m_Generation =
            l_Page->slots[l_SlotIndex].m_Generation;

        return l_Handle;
      }

      TexExport TexExport::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        TexExport l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = TexExport::TYPE_ID;

        return l_Handle;
      }

      bool TexExport::is_alive() const
      {
        if (m_Data.m_Type != TexExport::TYPE_ID) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
            l_Page->mutex);
        return m_Data.m_Type == TexExport::TYPE_ID &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t TexExport::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      TexExport::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      TexExport TexExport::find_by_name(Low::Util::Name p_Name)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:FIND_BY_NAME
        // LOW_CODEGEN::END::CUSTOM:FIND_BY_NAME

        Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
        return Low::Util::Handle::DEAD;
      }

      TexExport TexExport::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        TexExport l_Handle = make(p_Name);
        l_Handle.set_staging_buffer(get_staging_buffer());
        l_Handle.set_frame_index(get_frame_index());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      TexExport TexExport::duplicate(TexExport p_Handle,
                                     Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      TexExport::_duplicate(Low::Util::Handle p_Handle,
                            Low::Util::Name p_Name)
      {
        TexExport l_TexExport = p_Handle.get_id();
        return l_TexExport.duplicate(p_Name);
      }

      void TexExport::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["frame_index"] = get_frame_index();
        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void TexExport::serialize(Low::Util::Handle p_Handle,
                                Low::Util::Yaml::Node &p_Node)
      {
        TexExport l_TexExport = p_Handle.get_id();
        l_TexExport.serialize(p_Node);
      }

      Low::Util::Handle
      TexExport::deserialize(Low::Util::Yaml::Node &p_Node,
                             Low::Util::Handle p_Creator)
      {
        TexExport l_Handle = TexExport::make(N(TexExport));

        if (p_Node["staging_buffer"]) {
        }
        if (p_Node["frame_index"]) {
          l_Handle.set_frame_index(
              p_Node["frame_index"].as<uint32_t>());
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void TexExport::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64
      TexExport::observe(Low::Util::Name p_Observable,
                         Low::Util::Function<void(Low::Util::Handle,
                                                  Low::Util::Name)>
                             p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 TexExport::observe(Low::Util::Name p_Observable,
                             Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void TexExport::notify(Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        if (p_Observed.get_type() == TextureExport::TYPE_ID &&
            p_Observable == OBSERVABLE_DESTROY) {
          destroy();
        }
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void TexExport::_notify(Low::Util::Handle p_Observer,
                              Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
      {
        TexExport l_TexExport = p_Observer.get_id();
        l_TexExport.notify(p_Observed, p_Observable);
      }

      AllocatedBuffer &TexExport::get_staging_buffer() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<TexExport> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_staging_buffer
        // LOW_CODEGEN::END::CUSTOM:GETTER_staging_buffer

        return TYPE_SOA(TexExport, staging_buffer, AllocatedBuffer);
      }
      void TexExport::set_staging_buffer(AllocatedBuffer &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<TexExport> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_staging_buffer
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_staging_buffer

        // Set new value
        TYPE_SOA(TexExport, staging_buffer, AllocatedBuffer) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_staging_buffer
        // LOW_CODEGEN::END::CUSTOM:SETTER_staging_buffer

        broadcast_observable(N(staging_buffer));
      }

      uint32_t TexExport::get_frame_index() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<TexExport> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_frame_index
        // LOW_CODEGEN::END::CUSTOM:GETTER_frame_index

        return TYPE_SOA(TexExport, frame_index, uint32_t);
      }
      void TexExport::set_frame_index(uint32_t p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<TexExport> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_frame_index
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_frame_index

        // Set new value
        TYPE_SOA(TexExport, frame_index, uint32_t) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_frame_index
        // LOW_CODEGEN::END::CUSTOM:SETTER_frame_index

        broadcast_observable(N(frame_index));
      }

      Low::Util::Name TexExport::get_name() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<TexExport> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(TexExport, name, Low::Util::Name);
      }
      void TexExport::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<TexExport> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(TexExport, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      uint32_t TexExport::create_instance(
          u32 &p_PageIndex, u32 &p_SlotIndex,
          Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock)
      {
        LOCK_PAGES_WRITE(l_PagesLock);
        u32 l_Index = 0;
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        bool l_FoundIndex = false;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;

        for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
             ++l_PageIndex) {
          Low::Util::UniqueLock<Low::Util::Mutex> i_PageLock(
              ms_Pages[l_PageIndex]->mutex);
          for (l_SlotIndex = 0;
               l_SlotIndex < ms_Pages[l_PageIndex]->size;
               ++l_SlotIndex) {
            if (!ms_Pages[l_PageIndex]
                     ->slots[l_SlotIndex]
                     .m_Occupied) {
              l_FoundIndex = true;
              l_PageLock = std::move(i_PageLock);
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
          Low::Util::UniqueLock<Low::Util::Mutex> l_NewLock(
              ms_Pages[l_PageIndex]->mutex);
          l_PageLock = std::move(l_NewLock);
        }
        ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
        p_PageIndex = l_PageIndex;
        p_SlotIndex = l_SlotIndex;
        p_PageLock = std::move(l_PageLock);
        LOCK_UNLOCK(l_PagesLock);
        return l_Index;
      }

      u32 TexExport::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for TexExport.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, TexExport::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool TexExport::get_page_for_index(const u32 p_Index,
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

    } // namespace Vulkan
  } // namespace Renderer
} // namespace Low
