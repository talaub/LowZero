#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowMath.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace UI {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      struct Element;
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API ViewData
      {
        bool loaded;
        Util::Set<Util::UniqueId> elements;
        bool internal;
        bool view_template;
        Low::Math::Vector2 pixel_position;
        float rotation;
        float scale_multiplier;
        Low::Util::UniqueId unique_id;
        bool transform_dirty;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(ViewData);
        }
      };

      struct LOW_CORE_API View : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<View> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        View();
        View(uint64_t p_Id);
        View(View &p_Copy);

        static View make(Low::Util::Name p_Name);
        static Low::Util::Handle _make(Low::Util::Name p_Name);
        explicit View(const View &p_Copy)
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
        static View *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static View find_by_index(uint32_t p_Index);

        bool is_alive() const;

        static uint32_t get_capacity();

        void serialize(Low::Util::Yaml::Node &p_Node) const;

        View duplicate(Low::Util::Name p_Name) const;
        static View duplicate(View p_Handle, Low::Util::Name p_Name);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Name p_Name);

        static View find_by_name(Low::Util::Name p_Name);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Yaml::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          return p_Handle.get_type() == View::TYPE_ID &&
                 p_Handle.check_alive(ms_Slots, get_capacity());
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          View l_View = p_Handle.get_id();
          l_View.destroy();
        }

        bool is_loaded() const;
        void set_loaded(bool p_Value);

        Util::Set<Util::UniqueId> &get_elements() const;

        bool is_internal() const;

        bool is_view_template() const;
        void set_view_template(bool p_Value);

        Low::Math::Vector2 &pixel_position() const;
        void pixel_position(Low::Math::Vector2 &p_Value);

        float rotation() const;
        void rotation(float p_Value);

        float scale_multiplier() const;
        void scale_multiplier(float p_Value);

        Low::Util::UniqueId get_unique_id() const;

        bool is_transform_dirty() const;
        void set_transform_dirty(bool p_Value);

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        void serialize_elements(Util::Yaml::Node &p_Node);
        void add_element(Element p_Element);
        void remove_element(Element p_Element);
        void load_elements();
        void unload_elements();
        Low::Core::UI::View spawn_instance(Low::Util::Name p_Name);
        Low::Core::UI::Element
        find_element_by_name(Low::Util::Name p_Name);

      private:
        static uint32_t ms_Capacity;
        static uint32_t create_instance();
        static void increase_budget();
        void set_internal(bool p_Value);
        void set_unique_id(Low::Util::UniqueId p_Value);
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace UI
  }   // namespace Core
} // namespace Low
