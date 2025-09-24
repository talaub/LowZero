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

      struct LOW_CORE_API View : public Low::Util::Handle
      {
      public:
        struct Data
        {
        public:
          bool loaded;
          Util::Set<Util::UniqueId> elements;
          bool internal;
          bool view_template;
          Low::Math::Vector2 pixel_position;
          float rotation;
          float scale_multiplier;
          uint32_t layer_offset;
          Low::Util::UniqueId unique_id;
          bool transform_dirty;
          Low::Util::Name name;

          static size_t get_size()
          {
            return sizeof(Data);
          }
        };

      public:
        static Low::Util::SharedMutex ms_LivingMutex;
        static Low::Util::UniqueLock<Low::Util::SharedMutex>
            ms_PagesLock;
        static Low::Util::SharedMutex ms_PagesMutex;
        static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

        static Low::Util::List<View> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        View();
        View(uint64_t p_Id);
        View(View &p_Copy);

        static View make(Low::Util::Name p_Name);
        static Low::Util::Handle _make(Low::Util::Name p_Name);
        static View make(Low::Util::Name p_Name,
                         Low::Util::UniqueId p_UniqueId);
        explicit View(const View &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        static uint32_t living_count()
        {
          Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static View *living_instances()
        {
          Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          return ms_LivingInstances.data();
        }

        static View create_handle_by_index(u32 p_Index);

        static View find_by_index(uint32_t p_Index);
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
        void broadcast_observable(Low::Util::Name p_Observable) const;

        static void _notify(Low::Util::Handle p_Observer,
                            Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable);

        static uint32_t get_capacity();

        void serialize(Low::Util::Yaml::Node &p_Node) const;

        View duplicate(Low::Util::Name p_Name) const;
        static View duplicate(View p_Handle, Low::Util::Name p_Name);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Name p_Name);

        static View find_by_name(Low::Util::Name p_Name);
        static Low::Util::Handle
        _find_by_name(Low::Util::Name p_Name);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Yaml::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          View l_Handle = p_Handle.get_id();
          return l_Handle.is_alive();
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          View l_View = p_Handle.get_id();
          l_View.destroy();
        }

        bool is_loaded() const;
        void set_loaded(bool p_Value);
        void toggle_loaded();

        Util::Set<Util::UniqueId> &get_elements() const;

        bool is_internal() const;

        bool is_view_template() const;
        void set_view_template(bool p_Value);
        void toggle_view_template();

        Low::Math::Vector2 &pixel_position() const;
        void pixel_position(Low::Math::Vector2 &p_Value);
        void pixel_position(float p_X, float p_Y);
        void pixel_position_x(float p_Value);
        void pixel_position_y(float p_Value);

        float rotation() const;
        void rotation(float p_Value);

        float scale_multiplier() const;
        void scale_multiplier(float p_Value);

        uint32_t layer_offset() const;
        void layer_offset(uint32_t p_Value);

        Low::Util::UniqueId get_unique_id() const;

        bool is_transform_dirty() const;
        void set_transform_dirty(bool p_Value);
        void toggle_transform_dirty();
        void mark_transform_dirty();

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
        void set_internal(bool p_Value);
        void toggle_internal();
        void set_unique_id(Low::Util::UniqueId p_Value);

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace UI
  } // namespace Core
} // namespace Low
