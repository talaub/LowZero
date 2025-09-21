#include "LowRendererEditorImage.h"

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
    Util::Map<Util::Name, EditorImage> EditorImage::ms_Registry;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t EditorImage::TYPE_ID = 82;
    uint32_t EditorImage::ms_Capacity = 0u;
    uint32_t EditorImage::ms_PageSize = 0u;
    Low::Util::SharedMutex EditorImage::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        EditorImage::ms_PagesLock(EditorImage::ms_PagesMutex,
                                  std::defer_lock);
    Low::Util::List<EditorImage> EditorImage::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        EditorImage::ms_Pages;

    EditorImage::EditorImage() : Low::Util::Handle(0ull)
    {
    }
    EditorImage::EditorImage(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    EditorImage::EditorImage(EditorImage &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle EditorImage::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    EditorImage EditorImage::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      EditorImage l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = EditorImage::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<EditorImage> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, EditorImage, path,
                                 Low::Util::String))
          Low::Util::String();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, EditorImage, gpu,
                                 Low::Renderer::EditorImageGpu))
          Low::Renderer::EditorImageGpu();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, EditorImage, staging,
                                 Low::Renderer::EditorImageStaging))
          Low::Renderer::EditorImageStaging();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, EditorImage, state,
                                 Low::Renderer::TextureState))
          Low::Renderer::TextureState();
      ACCESSOR_TYPE_SOA(l_Handle, EditorImage, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_state(TextureState::UNLOADED);

      ms_Registry[p_Name] = l_Handle;
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void EditorImage::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<EditorImage> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // TODO: destroy gpu/staging
        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      _LOW_ASSERT(
          get_page_for_index(get_index(), l_PageIndex, l_SlotIndex));
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

    void EditorImage::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(EditorImage));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, EditorImage::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(EditorImage);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &EditorImage::is_alive;
      l_TypeInfo.destroy = &EditorImage::destroy;
      l_TypeInfo.serialize = &EditorImage::serialize;
      l_TypeInfo.deserialize = &EditorImage::deserialize;
      l_TypeInfo.find_by_index = &EditorImage::_find_by_index;
      l_TypeInfo.notify = &EditorImage::_notify;
      l_TypeInfo.find_by_name = &EditorImage::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &EditorImage::_make;
      l_TypeInfo.duplicate_default = &EditorImage::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &EditorImage::living_instances);
      l_TypeInfo.get_living_count = &EditorImage::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(EditorImage::Data, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImage l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImage> l_HandleLock(l_Handle);
          l_Handle.get_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, EditorImage,
                                            path, Low::Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImage l_Handle = p_Handle.get_id();
          l_Handle.set_path(*(Low::Util::String *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImage l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImage> l_HandleLock(l_Handle);
          *((Low::Util::String *)p_Data) = l_Handle.get_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: path
      }
      {
        // Property: gpu
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(gpu);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(EditorImage::Data, gpu);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::EditorImageGpu::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImage l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImage> l_HandleLock(l_Handle);
          l_Handle.get_gpu();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, EditorImage, gpu,
              Low::Renderer::EditorImageGpu);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImage l_Handle = p_Handle.get_id();
          l_Handle.set_gpu(*(Low::Renderer::EditorImageGpu *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImage l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImage> l_HandleLock(l_Handle);
          *((Low::Renderer::EditorImageGpu *)p_Data) =
              l_Handle.get_gpu();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: gpu
      }
      {
        // Property: staging
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(staging);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(EditorImage::Data, staging);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::EditorImageStaging::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImage l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImage> l_HandleLock(l_Handle);
          l_Handle.get_staging();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, EditorImage, staging,
              Low::Renderer::EditorImageStaging);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImage l_Handle = p_Handle.get_id();
          l_Handle.set_staging(
              *(Low::Renderer::EditorImageStaging *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImage l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImage> l_HandleLock(l_Handle);
          *((Low::Renderer::EditorImageStaging *)p_Data) =
              l_Handle.get_staging();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: staging
      }
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(EditorImage::Data, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            Low::Renderer::TextureStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImage l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImage> l_HandleLock(l_Handle);
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, EditorImage, state,
              Low::Renderer::TextureState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImage l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(Low::Renderer::TextureState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImage l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImage> l_HandleLock(l_Handle);
          *((Low::Renderer::TextureState *)p_Data) =
              l_Handle.get_state();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: state
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(EditorImage::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImage l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImage> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, EditorImage,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImage l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImage l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImage> l_HandleLock(l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void EditorImage::cleanup()
    {
      Low::Util::List<EditorImage> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle EditorImage::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    EditorImage EditorImage::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      EditorImage l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = EditorImage::TYPE_ID;

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

    EditorImage EditorImage::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      EditorImage l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = EditorImage::TYPE_ID;

      return l_Handle;
    }

    bool EditorImage::is_alive() const
    {
      if (m_Data.m_Type != EditorImage::TYPE_ID) {
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
      return m_Data.m_Type == EditorImage::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t EditorImage::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    EditorImage::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    EditorImage EditorImage::find_by_name(Low::Util::Name p_Name)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:FIND_BY_NAME
      auto pos = ms_Registry.find(p_Name);
      if (pos != ms_Registry.end()) {
        return pos->second;
      }
      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:FIND_BY_NAME

      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return Low::Util::Handle::DEAD;
    }

    EditorImage EditorImage::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      EditorImage l_Handle = make(p_Name);
      l_Handle.set_path(get_path());
      if (get_gpu().is_alive()) {
        l_Handle.set_gpu(get_gpu());
      }
      if (get_staging().is_alive()) {
        l_Handle.set_staging(get_staging());
      }
      l_Handle.set_state(get_state());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    EditorImage EditorImage::duplicate(EditorImage p_Handle,
                                       Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    EditorImage::_duplicate(Low::Util::Handle p_Handle,
                            Low::Util::Name p_Name)
    {
      EditorImage l_EditorImage = p_Handle.get_id();
      return l_EditorImage.duplicate(p_Name);
    }

    void EditorImage::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["path"] = get_path().c_str();
      if (get_gpu().is_alive()) {
        get_gpu().serialize(p_Node["gpu"]);
      }
      if (get_staging().is_alive()) {
        get_staging().serialize(p_Node["staging"]);
      }
      Low::Util::Serialization::serialize_enum(
          p_Node["state"],
          Low::Renderer::TextureStateEnumHelper::get_enum_id(),
          static_cast<uint8_t>(get_state()));
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void EditorImage::serialize(Low::Util::Handle p_Handle,
                                Low::Util::Yaml::Node &p_Node)
    {
      EditorImage l_EditorImage = p_Handle.get_id();
      l_EditorImage.serialize(p_Node);
    }

    Low::Util::Handle
    EditorImage::deserialize(Low::Util::Yaml::Node &p_Node,
                             Low::Util::Handle p_Creator)
    {
      EditorImage l_Handle = EditorImage::make(N(EditorImage));

      if (p_Node["path"]) {
        l_Handle.set_path(LOW_YAML_AS_STRING(p_Node["path"]));
      }
      if (p_Node["gpu"]) {
        l_Handle.set_gpu(Low::Renderer::EditorImageGpu::deserialize(
                             p_Node["gpu"], l_Handle.get_id())
                             .get_id());
      }
      if (p_Node["staging"]) {
        l_Handle.set_staging(
            Low::Renderer::EditorImageStaging::deserialize(
                p_Node["staging"], l_Handle.get_id())
                .get_id());
      }
      if (p_Node["state"]) {
        l_Handle.set_state(static_cast<Low::Renderer::TextureState>(
            Low::Util::Serialization::deserialize_enum(
                p_Node["state"])));
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void EditorImage::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 EditorImage::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 EditorImage::observe(Low::Util::Name p_Observable,
                             Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void EditorImage::notify(Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void EditorImage::_notify(Low::Util::Handle p_Observer,
                              Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
    {
      EditorImage l_EditorImage = p_Observer.get_id();
      l_EditorImage.notify(p_Observed, p_Observable);
    }

    Low::Util::String &EditorImage::get_path() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImage> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_path

      return TYPE_SOA(EditorImage, path, Low::Util::String);
    }
    void EditorImage::set_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_path(l_Val);
    }

    void EditorImage::set_path(Low::Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImage> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

      // Set new value
      TYPE_SOA(EditorImage, path, Low::Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_path

      broadcast_observable(N(path));
    }

    Low::Renderer::EditorImageGpu EditorImage::get_gpu() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImage> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gpu
      // LOW_CODEGEN::END::CUSTOM:GETTER_gpu

      return TYPE_SOA(EditorImage, gpu,
                      Low::Renderer::EditorImageGpu);
    }
    void EditorImage::set_gpu(Low::Renderer::EditorImageGpu p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImage> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gpu
      if (p_Value.is_alive()) {
        p_Value.set_editor_image_handle(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gpu

      // Set new value
      TYPE_SOA(EditorImage, gpu, Low::Renderer::EditorImageGpu) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gpu
      // LOW_CODEGEN::END::CUSTOM:SETTER_gpu

      broadcast_observable(N(gpu));
    }

    Low::Renderer::EditorImageStaging EditorImage::get_staging() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImage> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_staging
      // LOW_CODEGEN::END::CUSTOM:GETTER_staging

      return TYPE_SOA(EditorImage, staging,
                      Low::Renderer::EditorImageStaging);
    }
    void EditorImage::set_staging(
        Low::Renderer::EditorImageStaging p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImage> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_staging
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_staging

      // Set new value
      TYPE_SOA(EditorImage, staging,
               Low::Renderer::EditorImageStaging) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_staging
      // LOW_CODEGEN::END::CUSTOM:SETTER_staging

      broadcast_observable(N(staging));
    }

    Low::Renderer::TextureState EditorImage::get_state() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImage> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      return TYPE_SOA(EditorImage, state,
                      Low::Renderer::TextureState);
    }
    void EditorImage::set_state(Low::Renderer::TextureState p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImage> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      TYPE_SOA(EditorImage, state, Low::Renderer::TextureState) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
    }

    Low::Util::Name EditorImage::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImage> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(EditorImage, name, Low::Util::Name);
    }
    void EditorImage::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImage> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(EditorImage, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t EditorImage::create_instance(
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
          if (!ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied) {
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

    u32 EditorImage::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for EditorImage.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, EditorImage::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool EditorImage::get_page_for_index(const u32 p_Index,
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
