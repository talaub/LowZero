#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowCoreUiControllerInstance.h"
#include "LowCoreScriptClass.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace UI {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      struct CodeController
      {
        int x;
        // TODO: Implement
      };
      struct ScriptController
      {
        ScriptClass sclass;
      };
      struct ControllerValue
      {
        ControllerValue()
        {
        }
        union
        {
          CodeController code;
          ScriptController script;
        };
      };

      enum class ControllerType
      {
        Code,
        Script
      };
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API Controller : public Low::Util::Handle
      {
      public:
        struct Data
        {
        public:
          ControllerValue value;
          ControllerType type;
          Low::Util::Name name;

          static size_t get_size()
          {
            return sizeof(Data);
          }
        };

      private:
        static u16 ms_TypeId;

      public:
        static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

        static Low::Util::List<Controller> ms_LivingInstances;

        const static Low::Util::TypeIdentifier IDENTIFIER;

        [[nodiscard]] static u16 type_id()
        {
          return ms_TypeId;
        }

      private:
        static Controller make(Low::Util::Name p_Name);
        static Low::Util::Handle _make(Low::Util::Name p_Name);

      public:
        explicit Controller(const Controller &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        Controller(u64 p_Id) : Low::Util::Handle(p_Id)
        {
        }
        Controller() : Low::Util::Handle()
        {
        }
        Controller(Low::Util::Handle p_Handle)
            : Low::Util::Handle(p_Handle.get_id())
        {
        }

        using Handle::operator=;

        Controller &operator=(const Controller &) = default;
        Controller &operator=(Controller &&) noexcept = default;

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static Controller *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static Controller create_handle_by_index(u32 p_Index);

        static Controller find_by_index(uint32_t p_Index);
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

        Controller duplicate(Low::Util::Name p_Name) const;
        static Controller duplicate(Controller p_Handle,
                                    Low::Util::Name p_Name);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Name p_Name);

        static Controller find_by_name(Low::Util::Name p_Name);
        static Low::Util::Handle
        _find_by_name(Low::Util::Name p_Name);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Serial::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Serial::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          Controller l_Handle = p_Handle.get_id();
          return l_Handle.is_alive();
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          Controller l_Controller = p_Handle.get_id();
          l_Controller.destroy();
        }

        ControllerValue &get_value() const;
        void set_value(ControllerValue &p_Value);

        ControllerType get_type() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static Low::Core::UI::Controller
        make_code(Low::Util::Name p_Name);
        static Low::Core::UI::Controller
        make_script(Low::Util::Name p_Name,
                    Low::Core::Scripting::Class p_Class);
        Low::Core::UI::ControllerInstance spawn_instance();
        static Low::Core::UI::Controller find_by_scriptclass(
            Low::Core::Scripting::Class p_ScriptClass);
        bool is_script_controller() const;
        void update_instances();
        static bool get_page_for_index(const u32 p_Index,
                                       u32 &p_PageIndex,
                                       u32 &p_SlotIndex);

      private:
        static u32 ms_Capacity;
        static u32 ms_PageSize;
        static u32 create_instance(u32 &p_PageIndex,
                                   u32 &p_SlotIndex);
        static u32 create_page();
        void set_type(ControllerType p_Value);

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace UI
  } // namespace Core
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
