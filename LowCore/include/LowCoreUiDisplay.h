#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowCoreUiElement.h"

#include "LowMath.h"

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
          Math::Vector3 position;
          Math::Quaternion rotation;
          Math::Vector3 scale;
          uint64_t parent;
          uint64_t parent_uid;
          Util::List<uint64_t> children;
          Math::Vector3 world_position;
          Math::Quaternion world_rotation;
          Math::Vector3 world_scale;
          Math::Matrix4x4 world_matrix;
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
          static uint8_t *ms_Buffer;
          static Low::Util::Instances::Slot *ms_Slots;

          static Low::Util::List<Display> ms_LivingInstances;

          const static uint16_t TYPE_ID;

          Display();
          Display(uint64_t p_Id);
          Display(Display &p_Copy);

          static Display make(Low::Core::UI::Element p_Element);
          static Low::Util::Handle _make(Low::Util::Handle p_Element);
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
            return p_Handle.get_type() == Display::TYPE_ID &&
                   p_Handle.check_alive(ms_Slots, get_capacity());
          }

          static void destroy(Low::Util::Handle p_Handle)
          {
            _LOW_ASSERT(is_alive(p_Handle));
            Display l_Display = p_Handle.get_id();
            l_Display.destroy();
          }

          Math::Vector3 &position() const;
          void position(Math::Vector3 &p_Value);

          Math::Quaternion &rotation() const;
          void rotation(Math::Quaternion &p_Value);

          Math::Vector3 &scale() const;
          void scale(Math::Vector3 &p_Value);

          uint64_t get_parent() const;
          void set_parent(uint64_t p_Value);

          uint64_t get_parent_uid() const;

          Util::List<uint64_t> &get_children() const;

          Math::Vector3 &get_world_position();

          Math::Quaternion &get_world_rotation();

          Math::Vector3 &get_world_scale();

          Math::Matrix4x4 &get_world_matrix();

          bool is_world_updated() const;
          void set_world_updated(bool p_Value);

          Low::Core::UI::Element get_element() const;
          void set_element(Low::Core::UI::Element p_Value);

          Low::Util::UniqueId get_unique_id() const;

          bool is_dirty() const;
          void set_dirty(bool p_Value);

          bool is_world_dirty() const;
          void set_world_dirty(bool p_Value);

          void recalculate_world_transform();

        private:
          static uint32_t ms_Capacity;
          static uint32_t create_instance();
          static void increase_budget();
          void set_parent_uid(uint64_t p_Value);
          void set_world_position(Math::Vector3 &p_Value);
          void set_world_rotation(Math::Quaternion &p_Value);
          void set_world_scale(Math::Vector3 &p_Value);
          void set_world_matrix(Math::Matrix4x4 &p_Value);
          void set_unique_id(Low::Util::UniqueId p_Value);
        };
      } // namespace Component
    }   // namespace UI
  }     // namespace Core
} // namespace Low
