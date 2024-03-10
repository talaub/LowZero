#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowCoreUiElement.h"

#include "LowCoreTexture2D.h"
#include "LowRenderer.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace UI {
      namespace Component {
        // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
        // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

        struct LOW_CORE_API ImageData
        {
          Low::Core::Texture2D texture;
          Renderer::Material renderer_material;
          Low::Core::UI::Element element;
          Low::Util::UniqueId unique_id;

          static size_t get_size()
          {
            return sizeof(ImageData);
          }
        };

        struct LOW_CORE_API Image : public Low::Util::Handle
        {
        public:
          static uint8_t *ms_Buffer;
          static Low::Util::Instances::Slot *ms_Slots;

          static Low::Util::List<Image> ms_LivingInstances;

          const static uint16_t TYPE_ID;

          Image();
          Image(uint64_t p_Id);
          Image(Image &p_Copy);

          static Image make(Low::Core::UI::Element p_Element);
          static Low::Util::Handle _make(Low::Util::Handle p_Element);
          explicit Image(const Image &p_Copy)
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
          static Image *living_instances()
          {
            return ms_LivingInstances.data();
          }

          static Image find_by_index(uint32_t p_Index);

          bool is_alive() const;

          static uint32_t get_capacity();

          void serialize(Low::Util::Yaml::Node &p_Node) const;

          Image duplicate(Low::Core::UI::Element p_Entity) const;
          static Image duplicate(Image p_Handle,
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
            return p_Handle.get_type() == Image::TYPE_ID &&
                   p_Handle.check_alive(ms_Slots, get_capacity());
          }

          static void destroy(Low::Util::Handle p_Handle)
          {
            _LOW_ASSERT(is_alive(p_Handle));
            Image l_Image = p_Handle.get_id();
            l_Image.destroy();
          }

          Low::Core::Texture2D get_texture() const;
          void set_texture(Low::Core::Texture2D p_Value);

          Renderer::Material get_renderer_material() const;

          Low::Core::UI::Element get_element() const;
          void set_element(Low::Core::UI::Element p_Value);

          Low::Util::UniqueId get_unique_id() const;

        private:
          static uint32_t ms_Capacity;
          static uint32_t create_instance();
          static void increase_budget();
          void set_renderer_material(Renderer::Material p_Value);
          void set_unique_id(Low::Util::UniqueId p_Value);
        };

        // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
        // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      } // namespace Component
    }   // namespace UI
  }     // namespace Core
} // namespace Low
