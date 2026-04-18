#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace Scripting {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API Module : public Low::Util::Handle
      {
      public:
        struct Data
        {
        public:
          Low::Util::List<uint64_t> scripts;
          char *as_module;
          Low::Util::List<uint64_t> classes;
          uint32_t reload_index;
          Low::Util::List<char *> ticking_functions;
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

        static Low::Util::List<Module> ms_LivingInstances;

        const static Low::Util::TypeIdentifier IDENTIFIER;

        [[nodiscard]] static u16 type_id()
        {
          return ms_TypeId;
        }

        static Module make(Low::Util::Name p_Name);
        static Low::Util::Handle _make(Low::Util::Name p_Name);
        explicit Module(const Module &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        Module(u64 p_Id) : Low::Util::Handle(p_Id)
        {
        }
        Module() : Low::Util::Handle()
        {
        }
        Module(Low::Util::Handle p_Handle)
            : Low::Util::Handle(p_Handle.get_id())
        {
        }

        using Handle::operator=;

        Module &operator=(const Module &) = default;
        Module &operator=(Module &&) noexcept = default;

        static uint32_t living_count()
        {
          Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static Module *living_instances()
        {
          Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          return ms_LivingInstances.data();
        }

        static Module create_handle_by_index(u32 p_Index);

        static Module find_by_index(uint32_t p_Index);
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

        Module duplicate(Low::Util::Name p_Name) const;
        static Module duplicate(Module p_Handle,
                                Low::Util::Name p_Name);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Name p_Name);

        static Module find_by_name(Low::Util::Name p_Name);
        static Low::Util::Handle
        _find_by_name(Low::Util::Name p_Name);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Serial::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Serial::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          Module l_Handle = p_Handle.get_id();
          return l_Handle.is_alive();
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          Module l_Module = p_Handle.get_id();
          l_Module.destroy();
        }

        Low::Util::List<uint64_t> &get_scripts() const;

        char *get_as_module() const;
        void set_as_module(char *p_Value);

        Low::Util::List<uint64_t> &get_classes() const;

        uint32_t get_reload_index() const;
        void set_reload_index(uint32_t p_Value);

        Low::Util::List<char *> &get_ticking_functions() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        uint64_t find_class_by_name(Low::Util::Name p_Name);
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

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Scripting
  } // namespace Core
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE
namespace Low {
  namespace Core {
    typedef Scripting::Module ScriptModule;
  }
} // namespace Low
// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
