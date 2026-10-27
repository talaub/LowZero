#include "LowEditorVisualScriptNodes.h"

#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace GameplaySystemNodes {
        namespace {
          static ImU32 g_EventColor = IM_COL32(100, 200, 100, 255);

          struct BeginPlayNode : public Node
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_gameplay_system_begin_play);
            }

            virtual Util::String
            get_title(const Graph &) const override
            {
              return "Begin Play";
            }

            virtual Util::String
            get_subtitle(const Graph &) const override
            {
              return "GameplaySystem";
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "GameplaySystem";
            }

            virtual Util::String
            get_icon(const Graph &) const override
            {
              return ICON_LC_PLAY;
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_EventColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {

              Pin l_ExecOutMetadata = make_execution_pin("Then");
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_ExecOutMetadata),
                  p_Schema);
            }

            virtual void
            compile(Graph &p_Graph,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(id, "Then");
              p_Graph.continue_compilation(l_ThenPin->id,
                                           p_CompileContext);
            }
          };

          struct TickNode : public Node
          {
            Util::String get_icon(const Graph &) const override
            {
              return ICON_LC_TIMER;
            }

            virtual Util::Name get_name() const override
            {
              return N(vs_gameplay_system_tick);
            }

            virtual Util::String
            get_title(const Graph &) const override
            {
              return "Tick";
            }

            virtual Util::String
            get_subtitle(const Graph &) const override
            {
              return "GameplaySystem";
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "GameplaySystem";
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_EventColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {

              Pin l_ExecOutMetadata = make_execution_pin("Then");
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_ExecOutMetadata),
                  p_Schema);

              Pin l_DeltaMetadata = make_number_pin("Delta Time");
              l_DeltaMetadata.show_default_value_when_unlinked =
                  false;
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_DeltaMetadata),
                  p_Schema);
            }

            virtual void
            compile(Graph &p_Graph,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(id, "Then");
              p_Graph.continue_compilation(l_ThenPin->id,
                                           p_CompileContext);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_Graph;
              (void)id;
              (void)p_PinId;
              p_CompileContext.main_code.append("p_Delta");
            }
          };

        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          const Util::Name l_BeginPlayNodeType =
              p_Graph.register_node_type<BeginPlayNode>();
          const Util::Name l_TickNodeType =
              p_Graph.register_node_type<TickNode>();

          {
            NodeSpawnEntry l_Entry;
            l_Entry.id = N(vs_spawn_gameplay_system_begin_play);
            l_Entry.category = "GameplaySystem";
            l_Entry.title = "Begin Play";
            l_Entry.subtitle = "GameplaySystem";
            l_Entry.search_text = "begin play start gameplay system";
            l_Entry.node_type = l_BeginPlayNodeType;
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
            l_Entry.node_type = l_TickNodeType;
            p_Graph.register_spawn_entry(l_Entry);
          }
        }
      } // namespace GameplaySystemNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
