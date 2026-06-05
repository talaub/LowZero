#pragma once

#include "LowEditorVisualScripting.h"
#include <memory>

namespace Low {
  namespace Editor {
    namespace VisualScript {
      struct ContextDefinition;
      struct ContextRegistry;

      struct CanvasDropAction
      {
        Util::String label;
        std::function<void()> execute;

        bool is_valid() const
        {
          return !label.empty() && (bool)execute;
        }
      };

      struct Document
      {
        Util::String path;
        Util::String output_path;
        Util::Name context;
        Util::Name compile_profile;
        std::unique_ptr<CompileProfileSettings> compile_settings;
        Graph graph;
        Schema schema;
        GraphRenderer renderer;
        NodeGraphEditorController controller;
        NodeGraphCanvas canvas;
        NodeGraphEditorState state;

        Document() : schema(graph), renderer(graph)
        {
        }

        void initialize()
        {
          schema.set_graph(graph);
          renderer.set_graph(graph);
        }

        void apply_context(const ContextDefinition &p_Context);
        void apply_context(
            const ContextDefinition &p_Context,
            const CompileProfileRegistry &p_ProfileRegistry);
        bool initialize_compile_settings(
            const CompileProfileRegistry &p_ProfileRegistry);
        bool load_from_path(const Util::String &p_Path);
        bool load_from_path(const Util::String &p_Path,
                            const ContextRegistry &p_ContextRegistry);
        bool load_from_path(
            const Util::String &p_Path,
            const ContextRegistry &p_ContextRegistry,
            const CompileProfileRegistry &p_ProfileRegistry);
        bool save();
        bool save_as(const Util::String &p_Path);
        bool compile(CompileContext &p_Context);
        bool compile(CompileContext &p_Context,
                     const CompileProfileRegistry &p_ProfileRegistry);
        bool compile_and_write();
        bool compile_and_write(
            const CompileProfileRegistry &p_ProfileRegistry);

        template <typename T> T *get_compile_settings()
        {
          return dynamic_cast<T *>(compile_settings.get());
        }

        template <typename T> const T *get_compile_settings() const
        {
          return dynamic_cast<const T *>(compile_settings.get());
        }
      };

      struct LOW_EDITOR_API ContextDefinition
      {
        virtual ~ContextDefinition() = default;

        virtual Util::Name get_name() const = 0;
        virtual Util::Name get_default_compile_profile() const = 0;
        virtual void
        register_node_libraries(Graph &p_Graph) const = 0;
        virtual void
        build_default_template(Document &p_Document) const
        {
          (void)p_Document;
        }
        virtual void get_canvas_drop_payload_types(
            Util::List<Util::String> &p_Types) const
        {
          (void)p_Types;
        }
        virtual void get_canvas_drop_actions(
            Document &p_Document, const char *p_PayloadType,
            const void *p_Data, u32 p_DataSize,
            const Math::Vector2 &p_CanvasPosition,
            Util::List<CanvasDropAction> &p_Actions) const
        {
          (void)p_Document;
          (void)p_PayloadType;
          (void)p_Data;
          (void)p_DataSize;
          (void)p_CanvasPosition;
          (void)p_Actions;
        }
      };

      struct LOW_EDITOR_API ContextRegistry
      {
        Util::Map<Util::Name, ContextDefinition *> contexts;

        void register_context(ContextDefinition &p_Context);
        ContextDefinition *find_context(Util::Name p_ContextName);
        const ContextDefinition *
        find_context(Util::Name p_ContextName) const;
      };

      struct LOW_EDITOR_API DefaultContextDefinition
          : public ContextDefinition
      {
        virtual Util::Name get_name() const override;
        virtual Util::Name
        get_default_compile_profile() const override;
        virtual void
        register_node_libraries(Graph &p_Graph) const override;
      };

      struct LOW_EDITOR_API UiControllerContextDefinition
          : public ContextDefinition
      {
        virtual Util::Name get_name() const override;
        virtual Util::Name
        get_default_compile_profile() const override;
        virtual void
        register_node_libraries(Graph &p_Graph) const override;
        virtual void
        build_default_template(Document &p_Document) const override;
        virtual void get_canvas_drop_payload_types(
            Util::List<Util::String> &p_Types) const override;
        virtual void get_canvas_drop_actions(
            Document &p_Document, const char *p_PayloadType,
            const void *p_Data, u32 p_DataSize,
            const Math::Vector2 &p_CanvasPosition,
            Util::List<CanvasDropAction> &p_Actions) const
            override;
      };

      struct LOW_EDITOR_API GameplaySystemContextDefinition
          : public ContextDefinition
      {
        virtual Util::Name get_name() const override;
        virtual Util::Name
        get_default_compile_profile() const override;
        virtual void
        register_node_libraries(Graph &p_Graph) const override;
        virtual void
        build_default_template(Document &p_Document) const override;
      };

      LOW_EDITOR_API void
      register_builtin_contexts(ContextRegistry &p_ContextRegistry);

      LOW_EDITOR_API ContextRegistry &get_context_registry();
      LOW_EDITOR_API CompileProfileRegistry &
      get_compile_profile_registry();

      struct Editor
      {
        Document *document = nullptr;
        float sidebar_width = 260.0f;
        float min_sidebar_width = 160.0f;
        float max_sidebar_width = 480.0f;
        char new_variable_name[128] = "";
        PinType new_variable_type = PinType::Number;
        NumberSubtype new_variable_number_subtype =
            NumberSubtype::Float;
        StringSubtype new_variable_string_subtype =
            StringSubtype::String;
        Util::TypeIdentifier new_variable_handle_type;
        Util::List<CanvasDropAction> pending_canvas_drop_actions;

        bool embedded = false;
        bool sidebar_left = true;

        Editor() = default;
        Editor(Document &p_Document) : document(&p_Document)
        {
          load_document(p_Document);
        }

        void render(const float p_Delta);

        void load_document(Document &p_Document)
        {
          document = &p_Document;
          document->initialize();
        }

        void unload_document()
        {
          document = nullptr;
        }

        bool has_document() const
        {
          return document != nullptr;
        }

        Document *get_document()
        {
          return document;
        }

        const Document *get_document() const
        {
          return document;
        }

        Graph *get_graph()
        {
          return document ? &document->graph : nullptr;
        }

        const Graph *get_graph() const
        {
          return document ? &document->graph : nullptr;
        }

        Schema *get_schema()
        {
          return document ? &document->schema : nullptr;
        }

        const Schema *get_schema() const
        {
          return document ? &document->schema : nullptr;
        }

        GraphRenderer *get_renderer()
        {
          return document ? &document->renderer : nullptr;
        }

        const GraphRenderer *get_renderer() const
        {
          return document ? &document->renderer : nullptr;
        }

        NodeGraphCanvas *get_canvas()
        {
          return document ? &document->canvas : nullptr;
        }

        const NodeGraphCanvas *get_canvas() const
        {
          return document ? &document->canvas : nullptr;
        }

        NodeGraphEditorState *get_state()
        {
          return document ? &document->state : nullptr;
        }

        const NodeGraphEditorState *get_state() const
        {
          return document ? &document->state : nullptr;
        }

        void render(
            const char *p_Label,
            const Math::Vector2 &p_Size = Math::Vector2(0.0f, 0.0f));
      };
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
