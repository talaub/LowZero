#include "LowRendererImGuiImage.h"

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
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t ImGuiImage::TYPE_ID = 6;
      uint32_t ImGuiImage::ms_Capacity = 0u;
      uint32_t ImGuiImage::ms_PageSize = 0u;
      Low::Util::SharedMutex ImGuiImage::ms_LivingMutex;
      Low::Util::SharedMutex ImGuiImage::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          ImGuiImage::ms_PagesLock(ImGuiImage::ms_PagesMutex,
                                   std::defer_lock);
      Low::Util::List<ImGuiImage> ImGuiImage::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          ImGuiImage::ms_Pages;

      ImGuiImage::ImGuiImage() : Low::Util::Handle(0ull)
      {
      }
      ImGuiImage::ImGuiImage(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      ImGuiImage::ImGuiImage(ImGuiImage &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle ImGuiImage::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      ImGuiImage ImGuiImage::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        ImGuiImage l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = ImGuiImage::TYPE_ID;

        l_PageLock.unlock();

        Low::Util::HandleLock<ImGuiImage> l_HandleLock(l_Handle);

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, ImGuiImage, imgui_image,
                                   Backend::ImGuiImage))
            Backend::ImGuiImage();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, ImGuiImage, image,
                                   Resource::Image))
            Resource::Image();
        ACCESSOR_TYPE_SOA(l_Handle, ImGuiImage, name,
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

      void ImGuiImage::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<ImGuiImage> l_Lock(get_id());
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

          Backend::callbacks().imgui_image_cleanup(get_imgui_image());
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

      void ImGuiImage::initialize()
      {
        LOCK_PAGES_WRITE(l_PagesLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                      N(ImGuiImage));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, ImGuiImage::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(ImGuiImage);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &ImGuiImage::is_alive;
        l_TypeInfo.destroy = &ImGuiImage::destroy;
        l_TypeInfo.serialize = &ImGuiImage::serialize;
        l_TypeInfo.deserialize = &ImGuiImage::deserialize;
        l_TypeInfo.find_by_index = &ImGuiImage::_find_by_index;
        l_TypeInfo.notify = &ImGuiImage::_notify;
        l_TypeInfo.find_by_name = &ImGuiImage::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &ImGuiImage::_make;
        l_TypeInfo.duplicate_default = &ImGuiImage::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &ImGuiImage::living_instances);
        l_TypeInfo.get_living_count = &ImGuiImage::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: imgui_image
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(imgui_image);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ImGuiImage::Data, imgui_image);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ImGuiImage l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ImGuiImage> l_HandleLock(l_Handle);
            l_Handle.get_imgui_image();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ImGuiImage,
                                              imgui_image,
                                              Backend::ImGuiImage);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ImGuiImage l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ImGuiImage> l_HandleLock(l_Handle);
            *((Backend::ImGuiImage *)p_Data) =
                l_Handle.get_imgui_image();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: imgui_image
        }
        {
          // Property: image
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(image);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ImGuiImage::Data, image);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Resource::Image::TYPE_ID;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ImGuiImage l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ImGuiImage> l_HandleLock(l_Handle);
            l_Handle.get_image();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ImGuiImage,
                                              image, Resource::Image);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ImGuiImage l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ImGuiImage> l_HandleLock(l_Handle);
            *((Resource::Image *)p_Data) = l_Handle.get_image();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: image
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ImGuiImage::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ImGuiImage l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ImGuiImage> l_HandleLock(l_Handle);
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ImGuiImage,
                                              name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ImGuiImage l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ImGuiImage l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ImGuiImage> l_HandleLock(l_Handle);
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
          l_FunctionInfo.handleType = ImGuiImage::TYPE_ID;
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
            l_ParameterInfo.name = N(p_Image);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Resource::Image::TYPE_ID;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: make
        }
        {
          // Function: render
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(render);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Dimensions);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: render
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void ImGuiImage::cleanup()
      {
        Low::Util::List<ImGuiImage> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle ImGuiImage::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      ImGuiImage ImGuiImage::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        ImGuiImage l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = ImGuiImage::TYPE_ID;

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

      ImGuiImage ImGuiImage::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        ImGuiImage l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = ImGuiImage::TYPE_ID;

        return l_Handle;
      }

      bool ImGuiImage::is_alive() const
      {
        if (m_Data.m_Type != ImGuiImage::TYPE_ID) {
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
        return m_Data.m_Type == ImGuiImage::TYPE_ID &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t ImGuiImage::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      ImGuiImage::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      ImGuiImage ImGuiImage::find_by_name(Low::Util::Name p_Name)
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

      ImGuiImage ImGuiImage::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        ImGuiImage l_Handle = make(p_Name);
        if (get_image().is_alive()) {
          l_Handle.set_image(get_image());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      ImGuiImage ImGuiImage::duplicate(ImGuiImage p_Handle,
                                       Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      ImGuiImage::_duplicate(Low::Util::Handle p_Handle,
                             Low::Util::Name p_Name)
      {
        ImGuiImage l_ImGuiImage = p_Handle.get_id();
        return l_ImGuiImage.duplicate(p_Name);
      }

      void ImGuiImage::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        if (get_image().is_alive()) {
          get_image().serialize(p_Node["image"]);
        }
        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void ImGuiImage::serialize(Low::Util::Handle p_Handle,
                                 Low::Util::Yaml::Node &p_Node)
      {
        ImGuiImage l_ImGuiImage = p_Handle.get_id();
        l_ImGuiImage.serialize(p_Node);
      }

      Low::Util::Handle
      ImGuiImage::deserialize(Low::Util::Yaml::Node &p_Node,
                              Low::Util::Handle p_Creator)
      {
        ImGuiImage l_Handle = ImGuiImage::make(N(ImGuiImage));

        if (p_Node["imgui_image"]) {
        }
        if (p_Node["image"]) {
          l_Handle.set_image(Resource::Image::deserialize(
                                 p_Node["image"], l_Handle.get_id())
                                 .get_id());
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void ImGuiImage::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64
      ImGuiImage::observe(Low::Util::Name p_Observable,
                          Low::Util::Function<void(Low::Util::Handle,
                                                   Low::Util::Name)>
                              p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 ImGuiImage::observe(Low::Util::Name p_Observable,
                              Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void ImGuiImage::notify(Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void ImGuiImage::_notify(Low::Util::Handle p_Observer,
                               Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
      {
        ImGuiImage l_ImGuiImage = p_Observer.get_id();
        l_ImGuiImage.notify(p_Observed, p_Observable);
      }

      Backend::ImGuiImage &ImGuiImage::get_imgui_image() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ImGuiImage> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_imgui_image

        // LOW_CODEGEN::END::CUSTOM:GETTER_imgui_image

        return TYPE_SOA(ImGuiImage, imgui_image, Backend::ImGuiImage);
      }

      Resource::Image ImGuiImage::get_image() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ImGuiImage> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_image

        // LOW_CODEGEN::END::CUSTOM:GETTER_image

        return TYPE_SOA(ImGuiImage, image, Resource::Image);
      }
      void ImGuiImage::set_image(Resource::Image p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ImGuiImage> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_image

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_image

        // Set new value
        TYPE_SOA(ImGuiImage, image, Resource::Image) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_image

        // LOW_CODEGEN::END::CUSTOM:SETTER_image

        broadcast_observable(N(image));
      }

      Low::Util::Name ImGuiImage::get_name() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ImGuiImage> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(ImGuiImage, name, Low::Util::Name);
      }
      void ImGuiImage::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ImGuiImage> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(ImGuiImage, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      ImGuiImage ImGuiImage::make(Util::Name p_Name,
                                  Resource::Image p_Image)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make

        ImGuiImage l_ImGuiImage = ImGuiImage::make(p_Name);

        l_ImGuiImage.set_image(p_Image);

        Backend::callbacks().imgui_image_create(
            l_ImGuiImage.get_imgui_image(), p_Image.get_image());

        return l_ImGuiImage;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      void ImGuiImage::render(Math::UVector2 &p_Dimensions)
      {
        Low::Util::HandleLock<ImGuiImage> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_render

        if (!get_image().is_alive()) {
          LOW_LOG_WARN << "DEAD" << LOW_LOG_END;
        }
        Backend::callbacks().imgui_image_render(get_imgui_image(),
                                                p_Dimensions);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_render
      }

      uint32_t ImGuiImage::create_instance(
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

      u32 ImGuiImage::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for ImGuiImage.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, ImGuiImage::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool ImGuiImage::get_page_for_index(const u32 p_Index,
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

    } // namespace Interface
  } // namespace Renderer
} // namespace Low
