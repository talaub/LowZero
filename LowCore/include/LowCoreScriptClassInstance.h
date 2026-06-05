#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

#include <type_traits>
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace Scripting {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      struct Class;
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API ClassInstance : public Low::Util::Handle
      {
      public:
        struct Data
        {
        public:
          uint64_t script_class;
          uint32_t reload_index;
          char *ptr;
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

        static Low::Util::List<ClassInstance> ms_LivingInstances;

        const static Low::Util::TypeIdentifier IDENTIFIER;

        [[nodiscard]] static u16 type_id()
        {
          return ms_TypeId;
        }

      private:
        static ClassInstance make(Low::Util::Name p_Name);
        static Low::Util::Handle _make(Low::Util::Name p_Name);

      public:
        explicit ClassInstance(const ClassInstance &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        ClassInstance(u64 p_Id) : Low::Util::Handle(p_Id)
        {
        }
        ClassInstance() : Low::Util::Handle()
        {
        }
        ClassInstance(Low::Util::Handle p_Handle)
            : Low::Util::Handle(p_Handle.get_id())
        {
        }

        using Handle::operator=;

        ClassInstance &operator=(const ClassInstance &) = default;
        ClassInstance &operator=(ClassInstance &&) noexcept = default;

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static ClassInstance *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static ClassInstance create_handle_by_index(u32 p_Index);

        static ClassInstance find_by_index(uint32_t p_Index);
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

        ClassInstance duplicate(Low::Util::Name p_Name) const;
        static ClassInstance duplicate(ClassInstance p_Handle,
                                       Low::Util::Name p_Name);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Name p_Name);

        static ClassInstance find_by_name(Low::Util::Name p_Name);
        static Low::Util::Handle
        _find_by_name(Low::Util::Name p_Name);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Serial::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Serial::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          ClassInstance l_Handle = p_Handle.get_id();
          return l_Handle.is_alive();
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          ClassInstance l_ClassInstance = p_Handle.get_id();
          l_ClassInstance.destroy();
        }

        uint64_t get_script_class() const;
        void set_script_class(uint64_t p_Value);

        uint32_t get_reload_index() const;
        void set_reload_index(uint32_t p_Value);

        void set_ptr(char *p_Value);

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        bool needs_refresh();
        char *get_ptr();
        static Low::Core::Scripting::ClassInstance
        make(Low::Util::Name p_Name,
             Low::Core::Scripting::Class p_ScriptClass);
        static bool get_page_for_index(const u32 p_Index,
                                       u32 &p_PageIndex,
                                       u32 &p_SlotIndex);

      private:
        static u32 ms_Capacity;
        static u32 ms_PageSize;
        static u32 create_instance(u32 &p_PageIndex,
                                   u32 &p_SlotIndex);
        static u32 create_page();
        char *_ptr() const;
        char *spawn();

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE

      public:
        template <typename... TArgs>
        bool call_method(const char *p_Declaration,
                         TArgs &&...p_Args);

      private:
        bool call_method_internal(const char *p_Declaration,
                                  const void *const *p_Args,
                                  const char *const *p_TypeKeys,
                                  uint32_t p_ArgCount);
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      namespace Detail {
        template <typename T> struct ScriptArgTraits
        {
          static const char *key()
          {
            return "object";
          }
        };

        template <> struct ScriptArgTraits<bool>
        {
          static const char *key()
          {
            return "bool";
          }
        };

        template <> struct ScriptArgTraits<int8_t>
        {
          static const char *key()
          {
            return "int8";
          }
        };

        template <> struct ScriptArgTraits<uint8_t>
        {
          static const char *key()
          {
            return "uint8";
          }
        };

        template <> struct ScriptArgTraits<int16_t>
        {
          static const char *key()
          {
            return "int16";
          }
        };

        template <> struct ScriptArgTraits<uint16_t>
        {
          static const char *key()
          {
            return "uint16";
          }
        };

        template <> struct ScriptArgTraits<int32_t>
        {
          static const char *key()
          {
            return "int32";
          }
        };

        template <> struct ScriptArgTraits<uint32_t>
        {
          static const char *key()
          {
            return "uint32";
          }
        };

        template <> struct ScriptArgTraits<int64_t>
        {
          static const char *key()
          {
            return "int64";
          }
        };

        template <> struct ScriptArgTraits<uint64_t>
        {
          static const char *key()
          {
            return "uint64";
          }
        };

        template <> struct ScriptArgTraits<float>
        {
          static const char *key()
          {
            return "float";
          }
        };

        template <> struct ScriptArgTraits<double>
        {
          static const char *key()
          {
            return "double";
          }
        };

        template <typename T> struct ScriptArgTraits<T *>
        {
          static const char *key()
          {
            return "pointer";
          }
        };
      } // namespace Detail

      template <typename... TArgs>
      bool ClassInstance::call_method(const char *p_Declaration,
                                      TArgs &&...p_Args)
      {
        const void *l_Args[] = {(const void *)&p_Args..., nullptr};
        const char *l_TypeKeys[] = {
            Detail::ScriptArgTraits<std::remove_cv_t<
                std::remove_reference_t<TArgs>>>::key()...,
            nullptr};

        return call_method_internal(p_Declaration, l_Args, l_TypeKeys,
                                    sizeof...(TArgs));
      }
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Scripting
  } // namespace Core
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
