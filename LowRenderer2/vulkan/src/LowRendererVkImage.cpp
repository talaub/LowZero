#include "LowRendererVkImage.h"

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
#include "LowRendererVulkanImage.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t Image::TYPE_ID = 49;
      uint32_t Image::ms_Capacity = 0u;
      uint32_t Image::ms_PageSize = 0u;
      Low::Util::SharedMutex Image::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          Image::ms_PagesLock(Image::ms_PagesMutex, std::defer_lock);
      Low::Util::List<Image> Image::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *> Image::ms_Pages;

      Image::Image() : Low::Util::Handle(0ull)
      {
      }
      Image::Image(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      Image::Image(Image &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle Image::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Image Image::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        Image l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Image::TYPE_ID;

        l_PageLock.unlock();

        Low::Util::HandleLock<Image> l_HandleLock(l_Handle);

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Image, allocated_image,
                                   AllocatedImage)) AllocatedImage();
        ACCESSOR_TYPE_SOA(l_Handle, Image, depth, bool) = false;
        ACCESSOR_TYPE_SOA(l_Handle, Image, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        l_Handle.set_depth(false);
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Image::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<Image> l_Lock(get_id());
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          // TODO: Schedule image for deletion
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
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end();) {
          if (it->get_id() == get_id()) {
            it = ms_LivingInstances.erase(it);
          } else {
            it++;
          }
        }
        ms_PagesLock.unlock();
      }

      void Image::initialize()
      {
        LOCK_PAGES_WRITE(l_PagesLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                      N(Image));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Image::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Image);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Image::is_alive;
        l_TypeInfo.destroy = &Image::destroy;
        l_TypeInfo.serialize = &Image::serialize;
        l_TypeInfo.deserialize = &Image::deserialize;
        l_TypeInfo.find_by_index = &Image::_find_by_index;
        l_TypeInfo.notify = &Image::_notify;
        l_TypeInfo.find_by_name = &Image::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Image::_make;
        l_TypeInfo.duplicate_default = &Image::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Image::living_instances);
        l_TypeInfo.get_living_count = &Image::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: allocated_image
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(allocated_image);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Image::Data, allocated_image);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Image l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Image> l_HandleLock(l_Handle);
            l_Handle.get_allocated_image();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Image, allocated_image, AllocatedImage);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Image l_Handle = p_Handle.get_id();
            l_Handle.set_allocated_image(*(AllocatedImage *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Image l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Image> l_HandleLock(l_Handle);
            *((AllocatedImage *)p_Data) =
                l_Handle.get_allocated_image();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: allocated_image
        }
        {
          // Property: depth
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(depth);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Image::Data, depth);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Image l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Image> l_HandleLock(l_Handle);
            l_Handle.is_depth();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Image, depth,
                                              bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Image l_Handle = p_Handle.get_id();
            l_Handle.set_depth(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Image l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Image> l_HandleLock(l_Handle);
            *((bool *)p_Data) = l_Handle.is_depth();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: depth
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Image::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Image l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Image> l_HandleLock(l_Handle);
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Image, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Image l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Image l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Image> l_HandleLock(l_Handle);
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Function: unload
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(unload);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: unload
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void Image::cleanup()
      {
        Low::Util::List<Image> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle Image::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Image Image::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Image l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Image::TYPE_ID;

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

      Image Image::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Image l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Image::TYPE_ID;

        return l_Handle;
      }

      bool Image::is_alive() const
      {
        if (m_Data.m_Type != Image::TYPE_ID) {
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
        return m_Data.m_Type == Image::TYPE_ID &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Image::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle Image::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      Image Image::find_by_name(Low::Util::Name p_Name)
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

      Image Image::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        Image l_Handle = make(p_Name);
        l_Handle.set_allocated_image(get_allocated_image());
        l_Handle.set_depth(is_depth());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      Image Image::duplicate(Image p_Handle, Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle Image::_duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name)
      {
        Image l_Image = p_Handle.get_id();
        return l_Image.duplicate(p_Name);
      }

      void Image::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["depth"] = is_depth();
        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Image::serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node)
      {
        Image l_Image = p_Handle.get_id();
        l_Image.serialize(p_Node);
      }

      Low::Util::Handle
      Image::deserialize(Low::Util::Yaml::Node &p_Node,
                         Low::Util::Handle p_Creator)
      {
        Image l_Handle = Image::make(N(Image));

        if (p_Node["allocated_image"]) {
        }
        if (p_Node["depth"]) {
          l_Handle.set_depth(p_Node["depth"].as<bool>());
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void
      Image::broadcast_observable(Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 Image::observe(Low::Util::Name p_Observable,
                         Low::Util::Function<void(Low::Util::Handle,
                                                  Low::Util::Name)>
                             p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Image::observe(Low::Util::Name p_Observable,
                         Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Image::notify(Low::Util::Handle p_Observed,
                         Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Image::_notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
      {
        Image l_Image = p_Observer.get_id();
        l_Image.notify(p_Observed, p_Observable);
      }

      AllocatedImage &Image::get_allocated_image() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Image> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_allocated_image
        // LOW_CODEGEN::END::CUSTOM:GETTER_allocated_image

        return TYPE_SOA(Image, allocated_image, AllocatedImage);
      }
      void Image::set_allocated_image(AllocatedImage &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Image> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_allocated_image
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_allocated_image

        // Set new value
        TYPE_SOA(Image, allocated_image, AllocatedImage) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_allocated_image
        // LOW_CODEGEN::END::CUSTOM:SETTER_allocated_image

        broadcast_observable(N(allocated_image));
      }

      bool Image::is_depth() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Image> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_depth
        // LOW_CODEGEN::END::CUSTOM:GETTER_depth

        return TYPE_SOA(Image, depth, bool);
      }
      void Image::toggle_depth()
      {
        set_depth(!is_depth());
      }

      void Image::set_depth(bool p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Image> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_depth
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_depth

        // Set new value
        TYPE_SOA(Image, depth, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_depth
        // LOW_CODEGEN::END::CUSTOM:SETTER_depth

        broadcast_observable(N(depth));
      }

      Low::Util::Name Image::get_name() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Image> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Image, name, Low::Util::Name);
      }
      void Image::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Image> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(Image, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      bool Image::unload()
      {
        Low::Util::HandleLock<Image> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_unload
        return ImageUtil::Internal::destroy(get_allocated_image());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_unload
      }

      uint32_t Image::create_instance(
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

      u32 Image::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Image.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Image::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Image::get_page_for_index(const u32 p_Index,
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
