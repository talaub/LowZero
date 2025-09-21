#include "LowCoreFont.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowUtilResource.h"
#include "LowUtilJobManager.h"
#include "LowRenderer.h"
#include "ft2build.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

#include FT_FREETYPE_H
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    FT_Library g_FreeType;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Font::TYPE_ID = 36;
    uint32_t Font::ms_Capacity = 0u;
    uint32_t Font::ms_PageSize = 0u;
    Low::Util::SharedMutex Font::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        Font::ms_PagesLock(Font::ms_PagesMutex, std::defer_lock);
    Low::Util::List<Font> Font::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *> Font::ms_Pages;

    Font::Font() : Low::Util::Handle(0ull)
    {
    }
    Font::Font(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Font::Font(Font &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Font::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Font Font::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      Font l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = Font::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<Font> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Font, path, Util::String))
          Util::String();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, Font, glyphs,
          SINGLE_ARG(Util::Map<char, FontGlyph>)))
          Util::Map<char, FontGlyph>();
      ACCESSOR_TYPE_SOA(l_Handle, Font, font_size, float) = 0.0f;
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Font, state,
                                 ResourceState)) ResourceState();
      ACCESSOR_TYPE_SOA(l_Handle, Font, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Font::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<Font> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

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

    void Font::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      LOW_ASSERT(!FT_Init_FreeType(&g_FreeType),
                 "Failed to initialize FreeType");
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowCore), N(Font));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, Font::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Font);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Font::is_alive;
      l_TypeInfo.destroy = &Font::destroy;
      l_TypeInfo.serialize = &Font::serialize;
      l_TypeInfo.deserialize = &Font::deserialize;
      l_TypeInfo.find_by_index = &Font::_find_by_index;
      l_TypeInfo.notify = &Font::_notify;
      l_TypeInfo.find_by_name = &Font::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Font::_make;
      l_TypeInfo.duplicate_default = &Font::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Font::living_instances);
      l_TypeInfo.get_living_count = &Font::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Font::Data, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
          l_Handle.get_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Font, path,
                                            Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
          *((Util::String *)p_Data) = l_Handle.get_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: path
      }
      {
        // Property: glyphs
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(glyphs);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Font::Data, glyphs);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
          l_Handle.get_glyphs();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Font, glyphs,
              SINGLE_ARG(Util::Map<char, FontGlyph>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
          *((Util::Map<char, FontGlyph> *)p_Data) =
              l_Handle.get_glyphs();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: glyphs
      }
      {
        // Property: reference_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(reference_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Font::Data, reference_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
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
        // End property: reference_count
      }
      {
        // Property: font_size
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(font_size);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Font::Data, font_size);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
          l_Handle.get_font_size();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Font, font_size,
                                            float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
          *((float *)p_Data) = l_Handle.get_font_size();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: font_size
      }
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Font::Data, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Font, state,
                                            ResourceState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Font l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(ResourceState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
          *((ResourceState *)p_Data) = l_Handle.get_state();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: state
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Font::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Font, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Font l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
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
        l_FunctionInfo.handleType = Font::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Path);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::STRING;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      {
        // Function: is_loaded
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(is_loaded);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: is_loaded
      }
      {
        // Function: load
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(load);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: load
      }
      {
        // Function: _load
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(_load);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: _load
      }
      {
        // Function: unload
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(unload);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: unload
      }
      {
        // Function: _unload
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(_unload);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: _unload
      }
      {
        // Function: update
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(update);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: update
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Font::cleanup()
    {
      Low::Util::List<Font> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle Font::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    Font Font::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Font l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = Font::TYPE_ID;

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

    Font Font::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      Font l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = Font::TYPE_ID;

      return l_Handle;
    }

    bool Font::is_alive() const
    {
      if (m_Data.m_Type != Font::TYPE_ID) {
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
      return m_Data.m_Type == Font::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t Font::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle Font::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    Font Font::find_by_name(Low::Util::Name p_Name)
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

    Font Font::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      Font l_Handle = make(p_Name);
      l_Handle.set_path(get_path());
      l_Handle.set_glyphs(get_glyphs());
      l_Handle.set_reference_count(get_reference_count());
      l_Handle.set_font_size(get_font_size());
      l_Handle.set_state(get_state());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    Font Font::duplicate(Font p_Handle, Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle Font::_duplicate(Low::Util::Handle p_Handle,
                                       Low::Util::Name p_Name)
    {
      Font l_Font = p_Handle.get_id();
      return l_Font.duplicate(p_Name);
    }

    void Font::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      p_Node = get_path().c_str();
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Font::serialize(Low::Util::Handle p_Handle,
                         Low::Util::Yaml::Node &p_Node)
    {
      Font l_Font = p_Handle.get_id();
      l_Font.serialize(p_Node);
    }

    Low::Util::Handle Font::deserialize(Low::Util::Yaml::Node &p_Node,
                                        Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      Font l_Font = Font::make(LOW_YAML_AS_STRING(p_Node));
      return l_Font;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void
    Font::broadcast_observable(Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 Font::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 Font::observe(Low::Util::Name p_Observable,
                      Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void Font::notify(Low::Util::Handle p_Observed,
                      Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void Font::_notify(Low::Util::Handle p_Observer,
                       Low::Util::Handle p_Observed,
                       Low::Util::Name p_Observable)
    {
      Font l_Font = p_Observer.get_id();
      l_Font.notify(p_Observed, p_Observable);
    }

    Util::String &Font::get_path() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path

      // LOW_CODEGEN::END::CUSTOM:GETTER_path

      return TYPE_SOA(Font, path, Util::String);
    }
    void Font::set_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_path(l_Val);
    }

    void Font::set_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

      // Set new value
      TYPE_SOA(Font, path, Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path

      // LOW_CODEGEN::END::CUSTOM:SETTER_path

      broadcast_observable(N(path));
    }

    Util::Map<char, FontGlyph> &Font::get_glyphs() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_glyphs

      // LOW_CODEGEN::END::CUSTOM:GETTER_glyphs

      return TYPE_SOA(Font, glyphs,
                      SINGLE_ARG(Util::Map<char, FontGlyph>));
    }
    void Font::set_glyphs(Util::Map<char, FontGlyph> &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_glyphs

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_glyphs

      // Set new value
      TYPE_SOA(Font, glyphs, SINGLE_ARG(Util::Map<char, FontGlyph>)) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_glyphs

      // LOW_CODEGEN::END::CUSTOM:SETTER_glyphs

      broadcast_observable(N(glyphs));
    }

    uint32_t Font::get_reference_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:GETTER_reference_count

      return TYPE_SOA(Font, reference_count, uint32_t);
    }
    void Font::set_reference_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_reference_count

      // Set new value
      TYPE_SOA(Font, reference_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:SETTER_reference_count

      broadcast_observable(N(reference_count));
    }

    float Font::get_font_size() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_font_size

      // LOW_CODEGEN::END::CUSTOM:GETTER_font_size

      return TYPE_SOA(Font, font_size, float);
    }
    void Font::set_font_size(float p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_font_size

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_font_size

      // Set new value
      TYPE_SOA(Font, font_size, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_font_size

      // LOW_CODEGEN::END::CUSTOM:SETTER_font_size

      broadcast_observable(N(font_size));
    }

    ResourceState Font::get_state() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state

      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      return TYPE_SOA(Font, state, ResourceState);
    }
    void Font::set_state(ResourceState p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      TYPE_SOA(Font, state, ResourceState) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state

      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
    }

    Low::Util::Name Font::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Font, name, Low::Util::Name);
    }
    void Font::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Font, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    Font Font::make(Util::String &p_Path)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make

      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_path() == p_Path) {
          return *it;
        }
      }

      Util::String l_FileName =
          p_Path.substr(p_Path.find_last_of("/\\") + 1);
      Font l_Font = Font::make(LOW_NAME(l_FileName.c_str()));
      l_Font.set_path(p_Path);

      l_Font.set_reference_count(0);

      return l_Font;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    bool Font::is_loaded()
    {
      Low::Util::HandleLock<Font> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_loaded

      return get_state() == ResourceState::LOADED;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_loaded
    }

    void Font::load()
    {
      Low::Util::HandleLock<Font> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_load

      LOW_ASSERT(is_alive(), "Font was not alive on load");

      set_reference_count(get_reference_count() + 1);

      LOW_ASSERT(
          get_reference_count() > 0,
          "Increased Font reference count, but its not over 0. "
          "Something went wrong.");

      if (get_state() != ResourceState::UNLOADED) {
        return;
      }

      set_state(ResourceState::STREAMING);

      _load();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_load
    }

    void Font::_load()
    {
      Low::Util::HandleLock<Font> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION__load

      LOW_ASSERT(is_alive(), "Cannot load dead font handle");

      Util::String l_FullPath = Util::get_project().dataPath +
                                "\\resources\\fonts\\" + get_path();

      FT_Face l_Face;
      LOW_ASSERT(
          !FT_New_Face(g_FreeType, l_FullPath.c_str(), 0, &l_Face),
          "Unable to load font face (ttf)");

      FT_Set_Pixel_Sizes(l_Face, 0, 48);

      for (unsigned char c = 0; c < 128; c++) {
        // load character glyph
        bool i_Success = !FT_Load_Char(l_Face, c, FT_LOAD_RENDER);

        LOW_ASSERT_WARN(i_Success, "Could not load char from font");
        if (!i_Success) {
          continue;
        }

        Util::Resource::Image2D i_Image;
        i_Image.miplevel = 0;
        i_Image.dimensions.x = l_Face->glyph->bitmap.width;
        i_Image.dimensions.y = l_Face->glyph->bitmap.rows;
        i_Image.format = Util::Resource::Image2DFormat::R8;

        i_Image.data.resize(i_Image.dimensions.x *
                            i_Image.dimensions.y);
        memcpy(i_Image.data.data(), l_Face->glyph->bitmap.buffer,
               i_Image.data.size());

        // now store character for later use
        FontGlyph i_Glyph = {
            Renderer::upload_texture(N(FontGlyph), i_Image),
            glm::ivec2(l_Face->glyph->bitmap.width,
                       l_Face->glyph->bitmap.rows),
            glm::ivec2(l_Face->glyph->bitmap_left,
                       l_Face->glyph->bitmap_top),
            l_Face->glyph->advance.x};
        get_glyphs()[c] = i_Glyph;
      }

      int ascender = l_Face->ascender >> 6;
      int descender = l_Face->descender >> 6;

      // Calculate font height
      int font_height = ascender - descender;
      set_font_size((float)font_height);

      LOW_LOG_DEBUG << "Loaded font '" << get_path() << "'"
                    << LOW_LOG_END;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION__load
    }

    void Font::unload()
    {
      Low::Util::HandleLock<Font> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_unload

      set_reference_count(get_reference_count() - 1);

      LOW_ASSERT(get_reference_count() >= 0,
                 "Font reference count < 0. Something went wrong.");

      if (get_reference_count() <= 0) {
        _unload();
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_unload
    }

    void Font::_unload()
    {
      Low::Util::HandleLock<Font> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION__unload

      if (!is_loaded()) {
        return;
      }

      for (auto it = get_glyphs().begin();
           it != get_glyphs().end();) {
        it->second.rendererTexture.destroy();
        it = get_glyphs().erase(it);
      }

      _LOW_ASSERT(get_glyphs().empty());

      set_state(ResourceState::UNLOADED);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION__unload
    }

    void Font::update()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_update

      // LOW_CODEGEN::END::CUSTOM:FUNCTION_update
    }

    uint32_t Font::create_instance(
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

    u32 Font::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for Font.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, Font::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool Font::get_page_for_index(const u32 p_Index, u32 &p_PageIndex,
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

  } // namespace Core
} // namespace Low
