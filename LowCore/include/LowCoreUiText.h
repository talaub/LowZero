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
        // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

        struct LOW_CORE_API TextData
        {
          Util::String text;
          Core::Font font;
          Math::Color color;
          float size;
          Low::Core::UI::Element element;
          Low::Util::UniqueId unique_id;

          static size_t get_size()
          {
            return sizeof(TextData);
          }
        };

        struct LOW_CORE_API Text : public Low::Util::Handle
        {
        public:
          static uint8_t *ms_Buffer;
          static Low::Util::Instances::Slot *ms_Slots;

          static Low::Util::List<Text> ms_LivingInstances;

          const static uint16_t TYPE_ID;

          Text();
          Text(uint64_t p_Id);
          Text(Text &p_Copy);

          static Text make(Low::Core::UI::Element p_Element);
          static Low::Util::Handle _make(Low::Util::Handle p_Element);
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

          static Text find_by_index(uint32_t p_Index);

          bool is_alive() const;

          static uint32_t get_capacity();

          void serialize(Low::Util::Yaml::Node &p_Node) const;

          static void serialize(Low::Util::Handle p_Handle,
                                Low::Util::Yaml::Node &p_Node);
          static Low::Util::Handle
          deserialize(Low::Util::Yaml::Node &p_Node,
                      Low::Util::Handle p_Creator);
          static bool is_alive(Low::Util::Handle p_Handle)
          {
            return p_Handle.get_type() == Text::TYPE_ID &&
                   p_Handle.check_alive(ms_Slots, get_capacity());
          }

          static void destroy(Low::Util::Handle p_Handle)
          {
            _LOW_ASSERT(is_alive(p_Handle));
            Text l_Text = p_Handle.get_id();
            l_Text.destroy();
          }

          Util::String &get_text() const;
          void set_text(Util::String &p_Value);

          Core::Font get_font() const;
          void set_font(Core::Font p_Value);

          Math::Color &get_color() const;
          void set_color(Math::Color &p_Value);

          float get_size() const;
          void set_size(float p_Value);

          Low::Core::UI::Element get_element() const;
          void set_element(Low::Core::UI::Element p_Value);

          Low::Util::UniqueId get_unique_id() const;

        private:
          static uint32_t ms_Capacity;
          static uint32_t create_instance();
          static void increase_budget();
          void set_unique_id(Low::Util::UniqueId p_Value);
        };
      } // namespace Component
    }   // namespace UI
  }     // namespace Core
} // namespace Low
