#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowCoreUiWidgetInstance.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace UI {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      struct ComponentDescriptor
      {
        u16 typeId;
        Util::Serial::Node data;
      };
      struct ElementDescriptor
      {
        Util::Name name;
        Util::List<ComponentDescriptor> components;
        Util::List<ElementDescriptor> children;
      };
      enum class LoadState
      {
        Undefined,
        Unloaded,
        Loaded,
        Loading
      };
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API WidgetAsset : public Low::Util::Handle
      {
      public:
        struct Data
        {
        public:
          Low::Core::UI::LoadState state;
          Low::Util::List<Low::Core::UI::ElementDescriptor> content;
          Low::Util::String path;
          Low::Util::Name name;

          static size_t get_size()
          {
            return sizeof(Data);
          }
        };

      private:
        static u16 ms_TypeId;

      public:
        static Low::Util::SharedMutex ms_LivingMutex;
        static Low::Util::UniqueLock<Low::Util::SharedMutex>
            ms_PagesLock;
        static Low::Util::SharedMutex ms_PagesMutex;
        static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

        static Low::Util::List<WidgetAsset> ms_LivingInstances;

        const static Low::Util::TypeIdentifier IDENTIFIER;

        [[nodiscard]] static u16 type_id()
        {
          return ms_TypeId;
        }

        static WidgetAsset make(Low::Util::Name p_Name);
        static Low::Util::Handle _make(Low::Util::Name p_Name);
        explicit WidgetAsset(const WidgetAsset &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        WidgetAsset(u64 p_Id) : Low::Util::Handle(p_Id)
        {
        }
        WidgetAsset() : Low::Util::Handle()
        {
        }
        WidgetAsset(Low::Util::Handle p_Handle)
            : Low::Util::Handle(p_Handle.get_id())
        {
        }

        using Handle::operator=;

        WidgetAsset &operator=(const WidgetAsset &) = default;
        WidgetAsset &operator=(WidgetAsset &&) noexcept = default;

        static uint32_t living_count()
        {
          Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static WidgetAsset *living_instances()
        {
          Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          return ms_LivingInstances.data();
        }

        static WidgetAsset create_handle_by_index(u32 p_Index);

        static WidgetAsset find_by_index(uint32_t p_Index);
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

        void serialize(Low::Util::Serial::Node &p_Node) const;

        WidgetAsset duplicate(Low::Util::Name p_Name) const;
        static WidgetAsset duplicate(WidgetAsset p_Handle,
                                     Low::Util::Name p_Name);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Name p_Name);

        static WidgetAsset find_by_name(Low::Util::Name p_Name);
        static Low::Util::Handle
        _find_by_name(Low::Util::Name p_Name);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Serial::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Serial::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          WidgetAsset l_Handle = p_Handle.get_id();
          return l_Handle.is_alive();
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          WidgetAsset l_WidgetAsset = p_Handle.get_id();
          l_WidgetAsset.destroy();
        }

        void post_load(Low::Util::Serial::Node &p_Node);
        static void _post_load(Low::Util::Handle p_Handle,
                               Low::Util::Serial::Node &p_Node)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          WidgetAsset l_WidgetAsset = p_Handle.get_id();
          l_WidgetAsset.post_load(p_Node);
        }

        Low::Core::UI::LoadState get_state() const;
        void set_state(Low::Core::UI::LoadState p_Value);

        Low::Util::List<Low::Core::UI::ElementDescriptor> &
        get_content() const;
        void
        set_content(Low::Util::List<Low::Core::UI::ElementDescriptor>
                        &p_Value);

        Low::Util::String get_path() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        WidgetInstance
        spawn_instance(Low::Renderer::UiCanvas p_Canvas);
        void fill_content_from_instance(
            Low::Core::UI::WidgetInstance p_Instance);
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
        void set_path(Low::Util::String p_Value);
        void set_path(const char *p_Value);
        void parse_content(Low::Util::Serial::Node &p_Node);

        void
        parse_element(Low::Util::Serial::Node &p_Node,
                      Low::Core::UI::ElementDescriptor &p_Descriptor);

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace UI
  } // namespace Core
} // namespace Low
