#pragma once

#include "LowEditor.h"
#include "LowEditorApi.h"

#include "LowEditorMetadata.h"

#include "LowMath.h"
#include "LowUtilHandle.h"
#include <memory>

#define TYPE_MANAGER_EVENT(eventname)                                \
  virtual void eventname(Util::Handle p_Handle,                      \
                         TypeMetadata &p_Metatada)                   \
  {                                                                  \
  }

#define TYPE_MANAGER_HANDLER(eventname)                              \
  static void handle_##eventname(Util::Handle p_Handle,              \
                                 TypeMetadata &p_Metatada);          \
  static void handle_##eventname(Util::Handle p_Handle);

namespace Low {
  namespace Editor {
    struct TypeEditorFactory;
    template <typename T> struct TypeEditorFactory_Impl;

    struct TypeEventHandler
    {
      TYPE_MANAGER_EVENT(after_add)
      TYPE_MANAGER_EVENT(before_delete)
      TYPE_MANAGER_EVENT(after_save)
      TYPE_MANAGER_EVENT(before_save)
    };

    struct LOW_EDITOR_API TypeEditor
    {
      TypeEditor(Util::Handle p_Handle)
          : m_Handle(p_Handle),
            m_Metadata(get_type_metadata(p_Handle.get_type()))
      {
      }

      virtual ~TypeEditor() = default;

      virtual void render(const float p_Delta);

      void show();

      virtual Math::UVector2 get_edit_widget_dimensions()
      {
        return Math::UVector2{600, 600};
      }

    private:
      static void
      register_factory_for_type(u16 p_TypeId,
                                TypeEditorFactory *p_Factory);
      static void
      register_event_handler_for_type(u16 p_TypeId,
                                      TypeEventHandler *p_Handler);

    protected:
      void show_editor(Util::Name p_PropertyName);

      void show_line(const Util::String p_Label,
                     const Util::String p_Content);

    public:
      static void cleanup_registered_types();

      static bool has_factory(u16 p_TypeId);

      static TypeEditor *create(Util::Handle p_Handle);
      static std::unique_ptr<TypeEditor>
      create_unique(Util::Handle p_Handle);

      TYPE_MANAGER_HANDLER(after_add)
      TYPE_MANAGER_HANDLER(before_delete)
      TYPE_MANAGER_HANDLER(after_save)
      TYPE_MANAGER_HANDLER(before_save)

      template <typename T> static void register_type(u16 p_TypeId)
      {
        register_factory_for_type(p_TypeId,
                                  new TypeEditorFactory_Impl<T>);
        register_event_handler_for_type(p_TypeId,
                                        new TypeEventHandler);
      }

      template <typename T, typename H>
      static void register_type(u16 p_TypeId)
      {
        register_factory_for_type(p_TypeId,
                                  new TypeEditorFactory_Impl<T>);

        register_event_handler_for_type(p_TypeId, new H);
      }

    protected:
      Util::Handle m_Handle;
      TypeMetadata m_Metadata;
    };

    struct LOW_EDITOR_API TypeEditorFactory
    {
      virtual TypeEditor *create(Util::Handle p_Handle)
      {
        return new TypeEditor(p_Handle);
      }
      virtual std::unique_ptr<TypeEditor>
      create_unique(Util::Handle p_Handle)
      {
        return std::make_unique<TypeEditor>(p_Handle);
      }
    };

    template <typename T>
    struct TypeEditorFactory_Impl : public TypeEditorFactory
    {
      TypeEditor *create(Util::Handle p_Handle) override
      {
        return new T(p_Handle);
      }

      std::unique_ptr<TypeEditor>
      create_unique(Util::Handle p_Handle) override
      {
        return std::make_unique<T>(p_Handle);
      }
    };

  } // namespace Editor
} // namespace Low

#undef TYPE_MANAGER_EVENT
#undef TYPE_MANAGER_HANDLER
