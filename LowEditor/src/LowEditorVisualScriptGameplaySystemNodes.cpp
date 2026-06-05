#include "LowEditorVisualScriptNodes.h"

#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace GameplaySystemNodes {
        namespace {
          static ImU32 g_EventColor = IM_COL32(100, 200, 100, 255);

          struct BeginPlayNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_gameplay_system_begin_play);
            }

            virtual Util::String get_title(const Graph &,
                                           NodeId) const override
            {
              return "Begin Play";
            }

            virtual Util::String get_subtitle(const Graph &,
                                              NodeId) const override
            {
              return "GameplaySystem";
            }

            virtual Util::String get_category(const Graph &,
                                              NodeId) const override
            {
              return "GameplaySystem";
            }

            virtual Util::String get_icon(const Graph &,
                                          NodeId) const override
            {
              return ICON_LC_PLAY;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_EventColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              Editor::Pin l_ExecOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_ExecOutMetadata =
                  make_execution_pin_metadata("Then");
              p_Graph.add_pin(l_ExecOut, l_ExecOutMetadata, p_Schema);
            }

            virtual void
            compile(Graph &p_Graph, NodeId p_NodeId,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Then");
              p_Graph.continue_compilation(l_ThenPin->pin,
                                           p_CompileContext);
            }
          };

          struct TickNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_gameplay_system_tick);
            }

            virtual Util::String get_title(const Graph &,
                                           NodeId) const override
            {
              return "Tick";
            }

            virtual Util::String get_subtitle(const Graph &,
                                              NodeId) const override
            {
              return "GameplaySystem";
            }

            virtual Util::String get_category(const Graph &,
                                              NodeId) const override
            {
              return "GameplaySystem";
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_EventColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              Editor::Pin l_ExecOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_ExecOutMetadata =
                  make_execution_pin_metadata("Then");
              p_Graph.add_pin(l_ExecOut, l_ExecOutMetadata, p_Schema);

              Editor::Pin l_DeltaOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_DeltaMetadata =
                  make_number_pin_metadata("Delta Time");
              l_DeltaMetadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(l_DeltaOut, l_DeltaMetadata, p_Schema);
            }

            virtual void
            compile(Graph &p_Graph, NodeId p_NodeId,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Then");
              p_Graph.continue_compilation(l_ThenPin->pin,
                                           p_CompileContext);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_Graph;
              (void)p_NodeId;
              (void)p_PinId;
              p_CompileContext.main_code.append("p_Delta");
            }
          };

          static BeginPlayNodeClass g_BeginPlayNodeClass;
          static TickNodeClass g_TickNodeClass;
        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          p_Graph.register_node_class(g_BeginPlayNodeClass);
          p_Graph.register_node_class(g_TickNodeClass);

          {
            NodeSpawnEntry l_Entry;
            l_Entry.id = N(vs_spawn_gameplay_system_begin_play);
            l_Entry.category = "GameplaySystem";
            l_Entry.title = "Begin Play";
            l_Entry.subtitle = "GameplaySystem";
            l_Entry.search_text =
                "begin play start gameplay system";
            l_Entry.node_class = g_BeginPlayNodeClass.get_name();
            p_Graph.register_spawn_entry(l_Entry);
          }

          {
            NodeSpawnEntry l_Entry;
            l_Entry.id = N(vs_spawn_gameplay_system_tick);
            l_Entry.category = "GameplaySystem";
            l_Entry.title = "Tick";
            l_Entry.subtitle = "GameplaySystem";
            l_Entry.search_text =
                "tick update delta time gameplay system";
            l_Entry.node_class = g_TickNodeClass.get_name();
            p_Graph.register_spawn_entry(l_Entry);
          }
        }
      } // namespace GameplaySystemNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
