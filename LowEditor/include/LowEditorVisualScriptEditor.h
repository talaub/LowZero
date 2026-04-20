#pragma once

#include "LowEditorVisualScripting.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      struct Document
      {
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
      };

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
