#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowCoreUiElement.h"

#include "LowCoreTexture2D.h"
#include "LowCoreFont.h"
#include "LowRenderer.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace UI {
      namespace Component {
        // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

        enum class TextContentFitOptions
        {
          None,
          WordWrap,
          Fit
        };
        // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

        struct LOW_CORE_API Text : public Low::Util::Handle
        {
        public:
          struct Data
          {
          public:
            Low::Util::String text;
            Low::Core::Font font;
            Low::Math::Color color;
            float size;
            TextContentFitOptions content_fit_approach;
            Low::Core::UI::Element element;
            Low::Util::UniqueId unique_id;

            static size_t get_size()
            {
              return sizeof(Data);
            }
          };

        public:
          static Low::Util::UniqueLock<Low::Util::SharedMutex>
              ms_PagesLock;
          static Low::Util::SharedMutex ms_PagesMutex;
          static Low::Util::List<Low::Util::Instances::Page *>
              ms_Pages;

          static Low::Util::List<Text> ms_LivingInstances;

          const static uint16_t TYPE_ID;

          Text();
          Text(uint64_t p_Id);
          Text(Text &p_Copy);

          static Text make(Low::Core::UI::Element p_Element);
          static Low::Util::Handle _make(Low::Util::Handle p_Element);
          static Text make(Low::Core::UI::Element p_Element,
                           Low::Util::UniqueId p_UniqueId);
          explicit Text(const Text &p_Copy)
              : Low::Util::Handle(p_Copy.m_Id)
          {
          }

          void destroy();

          static void initialize();
          static void cleanup();

          static uint32_t living_count()
          {
            return static_cast<uint32_t>(ms_LivingInstances.size());
          }
          static Text *living_instances()
          {
            return ms_LivingInstances.data();
          }

          static Text create_handle_by_index(u32 p_Index);

          static Text find_by_index(uint32_t p_Index);
          static Low::Util::Handle _find_by_index(uint32_t p_Index);

          bool is_alive() const;

          u64 observe(Low::Util::Name p_Observable,
                      Low::Util::Handle p_Observer) const;
          u64 observe(Low::Util::Name p_Observable,
                      Low::Util::Function<void(Low::Util::Handle,
                                               Low::Util::Name)>
                          p_Observer) const;
          void notify(Low::Util::Handle p_Observed,
                      Low::Util::Name p_Observable);
          void
          broadcast_observable(Low::Util::Name p_Observable) const;

          static void _notify(Low::Util::Handle p_Observer,
                              Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable);

          static uint32_t get_capacity();

          void serialize(Low::Util::Yaml::Node &p_Node) const;

          Text duplicate(Low::Core::UI::Element p_Entity) const;
          static Text duplicate(Text p_Handle,
                                Low::Core::UI::Element p_Element);
          static Low::Util::Handle
          _duplicate(Low::Util::Handle p_Handle,
                     Low::Util::Handle p_Element);

          static void serialize(Low::Util::Handle p_Handle,
                                Low::Util::Yaml::Node &p_Node);
          static Low::Util::Handle
          deserialize(Low::Util::Yaml::Node &p_Node,
                      Low::Util::Handle p_Creator);
          static bool is_alive(Low::Util::Handle p_Handle)
          {
            Text l_Handle = p_Handle.get_id();
            return l_Handle.is_alive();
          }

          static void destroy(Low::Util::Handle p_Handle)
          {
            _LOW_ASSERT(is_alive(p_Handle));
            Text l_Text = p_Handle.get_id();
            l_Text.destroy();
          }

          Low::Util::String &get_text() const;
          void set_text(Low::Util::String &p_Value);
          void set_text(const char *p_Value);

          Low::Core::Font get_font() const;
          void set_font(Low::Core::Font p_Value);

          Low::Math::Color &get_color() const;
          void set_color(Low::Math::Color &p_Value);

          float get_size() const;
          void set_size(float p_Value);

          TextContentFitOptions get_content_fit_approach() const;
          void
          set_content_fit_approach(TextContentFitOptions p_Value);

          Low::Core::UI::Element get_element() const;
          void set_element(Low::Core::UI::Element p_Value);

          Low::Util::UniqueId get_unique_id() const;

          static bool get_page_for_index(const u32 p_Index,
                                         u32 &p_PageIndex,
                                         u32 &p_SlotIndex);

        private:
          static u32 ms_Capacity;
          static u32 ms_PageSize;
          static u32 create_instance(
              u32 &p_PageIndex, u32 &p_SlotIndex,
              Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock);
          static u32 create_page();
          void set_unique_id(Low::Util::UniqueId p_Value);

          // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
          // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
        };

        // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

        // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      } // namespace Component
    } // namespace UI
  } // namespace Core
} // namespace Low
