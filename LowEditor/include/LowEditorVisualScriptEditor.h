#pragma once

#include "LowEditorVisualScripting.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      struct ContextDefinition;
      struct ContextRegistry;

      struct Document
      {
        Util::String path;
        Util::String output_path;
        Util::Name context;
        Util::Name compile_profile;
        Graph graph;
        Schema schema;
        GraphRenderer renderer;
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
        bool load_from_path(const Util::String &p_Path);
        bool load_from_path(const Util::String &p_Path,
                            const ContextRegistry &p_ContextRegistry);
        bool save();
        bool save_as(const Util::String &p_Path);
        bool compile_and_write(
            const CompileProfileRegistry &p_ProfileRegistry);
      };

      struct LOW_EDITOR_API ContextDefinition
      {
        virtual ~ContextDefinition() = default;

        virtual Util::Name get_name() const = 0;
        virtual Util::Name get_default_compile_profile() const = 0;
        virtual void register_node_libraries(Graph &p_Graph) const = 0;
        virtual void build_default_template(Document &p_Document) const
        {
          (void)p_Document;
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
        virtual Util::Name get_default_compile_profile() const override;
        virtual void
        register_node_libraries(Graph &p_Graph) const override;
      };

      struct LOW_EDITOR_API UiControllerContextDefinition
          : public ContextDefinition
      {
        virtual Util::Name get_name() const override;
        virtual Util::Name get_default_compile_profile() const override;
        virtual void
        register_node_libraries(Graph &p_Graph) const override;
        virtual void
        build_default_template(Document &p_Document) const override;
      };

      LOW_EDITOR_API void
      register_builtin_contexts(ContextRegistry &p_ContextRegistry);

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

        void render(const char *p_Label,
                    const Math::Vector2 &p_Size =
                        Math::Vector2(0.0f, 0.0f));
      };
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
