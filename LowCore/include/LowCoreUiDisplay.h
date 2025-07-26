#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowCoreUiElement.h"

#include "LowMath.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace UI {
      namespace Component {
        // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

        // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

        struct LOW_CORE_API DisplayData
        {
          Low::Math::Vector2 pixel_position;
          float rotation;
          Low::Math::Vector2 pixel_scale;
          uint32_t layer;
          uint64_t parent;
          uint64_t parent_uid;
          Low::Util::List<uint64_t> children;
          Low::Math::Vector2 absolute_pixel_position;
          float absolute_rotation;
          Low::Math::Vector2 absolute_pixel_scale;
          uint32_t absolute_layer;
          Low::Math::Matrix4x4 world_matrix;
          bool world_updated;
          Low::Core::UI::Element element;
          Low::Util::UniqueId unique_id;
          bool dirty;
          bool world_dirty;

          static size_t get_size()
          {
            return sizeof(DisplayData);
          }
        };

        struct LOW_CORE_API Display : public Low::Util::Handle
        {
        public:
          static std::shared_mutex ms_BufferMutex;
          static uint8_t *ms_Buffer;
          static Low::Util::Instances::Slot *ms_Slots;

          static Low::Util::List<Display> ms_LivingInstances;

          const static uint16_t TYPE_ID;

          Display();
          Display(uint64_t p_Id);
          Display(Display &p_Copy);

          static Display make(Low::Core::UI::Element p_Element);
          static Low::Util::Handle _make(Low::Util::Handle p_Element);
          static Display make(Low::Core::UI::Element p_Element,
                              Low::Util::UniqueId p_UniqueId);
          explicit Display(const Display &p_Copy)
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
          static Display *living_instances()
          {
            return ms_LivingInstances.data();
          }

          static Display find_by_index(uint32_t p_Index);
          static Low::Util::Handle _find_by_index(uint32_t p_Index);

          bool is_alive() const;

          static uint32_t get_capacity();

          void serialize(Low::Util::Yaml::Node &p_Node) const;

          Display duplicate(Low::Core::UI::Element p_Entity) const;
          static Display duplicate(Display p_Handle,
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
            READ_LOCK(l_Lock);
            return p_Handle.get_type() == Display::TYPE_ID &&
                   p_Handle.check_alive(ms_Slots, get_capacity());
          }

          static void destroy(Low::Util::Handle p_Handle)
          {
            _LOW_ASSERT(is_alive(p_Handle));
            Display l_Display = p_Handle.get_id();
            l_Display.destroy();
          }

          Low::Math::Vector2 &pixel_position() const;
          void pixel_position(Low::Math::Vector2 &p_Value);
          void pixel_position(float p_X, float p_Y);
          void pixel_position_x(float p_Value);
          void pixel_position_y(float p_Value);

          float rotation() const;
          void rotation(float p_Value);

          Low::Math::Vector2 &pixel_scale() const;
          void pixel_scale(Low::Math::Vector2 &p_Value);
          void pixel_scale(float p_X, float p_Y);
          void pixel_scale_x(float p_Value);
          void pixel_scale_y(float p_Value);

          uint32_t layer() const;
          void layer(uint32_t p_Value);

          uint64_t get_parent() const;
          void set_parent(uint64_t p_Value);

          uint64_t get_parent_uid() const;

          Low::Util::List<uint64_t> &get_children() const;

          Low::Math::Vector2 &get_absolute_pixel_position();

          float get_absolute_rotation();

          Low::Math::Vector2 &get_absolute_pixel_scale();

          uint32_t get_absolute_layer();

          Low::Math::Matrix4x4 &get_world_matrix();

          bool is_world_updated() const;
          void set_world_updated(bool p_Value);
          void toggle_world_updated();

          Low::Core::UI::Element get_element() const;
          void set_element(Low::Core::UI::Element p_Value);

          Low::Util::UniqueId get_unique_id() const;

          bool is_dirty() const;
          void set_dirty(bool p_Value);
          void toggle_dirty();
          void mark_dirty();

          bool is_world_dirty() const;
          void set_world_dirty(bool p_Value);
          void toggle_world_dirty();
          void mark_world_dirty();

          void recalculate_world_transform();
          float get_absolute_layer_float();
          bool point_is_in_bounding_box(Low::Math::Vector2 &p_Point);

        private:
          static uint32_t ms_Capacity;
          static uint32_t create_instance();
          static void increase_budget();
          void set_parent_uid(uint64_t p_Value);
          void
          set_absolute_pixel_position(Low::Math::Vector2 &p_Value);
          void set_absolute_pixel_position(float p_X, float p_Y);
          void set_absolute_pixel_position_x(float p_Value);
          void set_absolute_pixel_position_y(float p_Value);
          void set_absolute_rotation(float p_Value);
          void set_absolute_pixel_scale(Low::Math::Vector2 &p_Value);
          void set_absolute_pixel_scale(float p_X, float p_Y);
          void set_absolute_pixel_scale_x(float p_Value);
          void set_absolute_pixel_scale_y(float p_Value);
          void set_absolute_layer(uint32_t p_Value);
          void set_world_matrix(Low::Math::Matrix4x4 &p_Value);
          void set_unique_id(Low::Util::UniqueId p_Value);

          // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
          // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
        };

        // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

        // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      } // namespace Component
    }   // namespace UI
  }     // namespace Core
} // namespace Low
