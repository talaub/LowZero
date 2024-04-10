#include "LowCoreFont.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

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
    uint8_t *Font::ms_Buffer = 0;
    Low::Util::Instances::Slot *Font::ms_Slots = 0;
    Low::Util::List<Font> Font::ms_LivingInstances =
        Low::Util::List<Font>();

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
      uint32_t l_Index = create_instance();

      Font l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Font::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Font, path, Util::String))
          Util::String();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Font, glyphs,
                              SINGLE_ARG(Util::Map<char, FontGlyph>)))
          Util::Map<char, FontGlyph>();
      ACCESSOR_TYPE_SOA(l_Handle, Font, font_size, float) = 0.0f;
      new (&ACCESSOR_TYPE_SOA(l_Handle, Font, state, ResourceState))
          ResourceState();
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

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const Font *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void Font::initialize()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      LOW_ASSERT(!FT_Init_FreeType(&g_FreeType),
                 "Failed to initialize FreeType");
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowCore), N(Font));

      initialize_buffer(&ms_Buffer, FontData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_Font);
      LOW_PROFILE_ALLOC(type_slots_Font);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Font);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Font::is_alive;
      l_TypeInfo.destroy = &Font::destroy;
      l_TypeInfo.serialize = &Font::serialize;
      l_TypeInfo.deserialize = &Font::deserialize;
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
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(FontData, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          l_Handle.get_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Font, path,
                                            Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(glyphs);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(FontData, glyphs);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          l_Handle.get_glyphs();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Font, glyphs,
              SINGLE_ARG(Util::Map<char, FontGlyph>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(reference_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(FontData, reference_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          return nullptr;
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(font_size);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(FontData, font_size);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          l_Handle.get_font_size();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Font, font_size,
                                            float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(FontData, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Font, state,
                                            ResourceState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Font l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(ResourceState *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(FontData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Font, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Font l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Font::cleanup()
    {
      Low::Util::List<Font> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Font);
      LOW_PROFILE_FREE(type_slots_Font);
    }

    Font Font::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Font l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = Font::TYPE_ID;

      return l_Handle;
    }

    bool Font::is_alive() const
    {
      return m_Data.m_Type == Font::TYPE_ID &&
             check_alive(ms_Slots, Font::get_capacity());
    }

    uint32_t Font::get_capacity()
    {
      return ms_Capacity;
    }

    Font Font::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
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

    Util::String &Font::get_path() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_path

      return TYPE_SOA(Font, path, Util::String);
    }
    void Font::set_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

      // Set new value
      TYPE_SOA(Font, path, Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_path
    }

    Util::Map<char, FontGlyph> &Font::get_glyphs() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_glyphs
      // LOW_CODEGEN::END::CUSTOM:GETTER_glyphs

      return TYPE_SOA(Font, glyphs,
                      SINGLE_ARG(Util::Map<char, FontGlyph>));
    }
    void Font::set_glyphs(Util::Map<char, FontGlyph> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_glyphs
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_glyphs

      // Set new value
      TYPE_SOA(Font, glyphs, SINGLE_ARG(Util::Map<char, FontGlyph>)) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_glyphs
      // LOW_CODEGEN::END::CUSTOM:SETTER_glyphs
    }

    uint32_t Font::get_reference_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_reference_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_reference_count

      return TYPE_SOA(Font, reference_count, uint32_t);
    }
    void Font::set_reference_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_reference_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_reference_count

      // Set new value
      TYPE_SOA(Font, reference_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_reference_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_reference_count
    }

    float Font::get_font_size() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_font_size
      // LOW_CODEGEN::END::CUSTOM:GETTER_font_size

      return TYPE_SOA(Font, font_size, float);
    }
    void Font::set_font_size(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_font_size
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_font_size

      // Set new value
      TYPE_SOA(Font, font_size, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_font_size
      // LOW_CODEGEN::END::CUSTOM:SETTER_font_size
    }

    ResourceState Font::get_state() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      return TYPE_SOA(Font, state, ResourceState);
    }
    void Font::set_state(ResourceState p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      TYPE_SOA(Font, state, ResourceState) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state
    }

    Low::Util::Name Font::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Font, name, Low::Util::Name);
    }
    void Font::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Font, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
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
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_loaded
      return get_state() == ResourceState::LOADED;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_loaded
    }

    void Font::load()
    {
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

    uint32_t Font::create_instance()
    {
      uint32_t l_Index = 0u;

      for (; l_Index < get_capacity(); ++l_Index) {
        if (!ms_Slots[l_Index].m_Occupied) {
          break;
        }
      }
      if (l_Index >= get_capacity()) {
        increase_budget();
      }
      ms_Slots[l_Index].m_Occupied = true;
      return l_Index;
    }

    void Font::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(FontData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(FontData, path) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FontData, path) * (l_Capacity)],
               l_Capacity * sizeof(Util::String));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(FontData, glyphs) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Util::Map<char, FontGlyph>))])
              Util::Map<char, FontGlyph>();
          *i_ValPtr = it->get_glyphs();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(FontData, reference_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FontData, reference_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(FontData, font_size) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(FontData, font_size) * (l_Capacity)],
            l_Capacity * sizeof(float));
      }
      {
        memcpy(&l_NewBuffer[offsetof(FontData, state) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FontData, state) * (l_Capacity)],
               l_Capacity * sizeof(ResourceState));
      }
      {
        memcpy(&l_NewBuffer[offsetof(FontData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FontData, name) * (l_Capacity)],
               l_Capacity * sizeof(Low::Util::Name));
      }
      for (uint32_t i = l_Capacity;
           i < l_Capacity + l_CapacityIncrease; ++i) {
        l_NewSlots[i].m_Occupied = false;
        l_NewSlots[i].m_Generation = 0;
      }
      free(ms_Buffer);
      free(ms_Slots);
      ms_Buffer = l_NewBuffer;
      ms_Slots = l_NewSlots;
      ms_Capacity = l_Capacity + l_CapacityIncrease;

      LOW_LOG_DEBUG << "Auto-increased budget for Font from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Core
} // namespace Low
