#include "LowCoreScripting.h"
#include "LowEditor.h"
#include "LowEditorVisualScriptNodes.h"

#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace EnumNodes {
        namespace {
          static ImU32 g_EnumColor = IM_COL32(150, 100, 200, 255);
          static const char *g_ValuePinName = "Value";

          struct EnumLiteralNode : public TypedNode
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_enum_literal);
            }

            virtual Util::String
            get_title(const Graph &p_Graph) const override
            {
              const auto *l_Node = this;
              if (!l_Node || (u64)l_Node->handle_type == 0) {
                return "Enum literal";
              }
              return prettify_name(
                  Core::Scripting::find_registered_enum_checked(
                      l_Node->handle_type)
                      .bind_name);
            }

            virtual Util::String
            get_subtitle(const Graph &) const override
            {
              return "Enum";
            }

            virtual Util::String
            get_category(const Graph &p_Graph) const override
            {
              const auto *l_Node = this;
              if (!l_Node || (u64)l_Node->handle_type == 0) {
                return "Enum";
              }
              const Core::Scripting::VisualScriptTypeInfo &l_VsInfo =
                  Core::Scripting::find_registered_enum_checked(
                      l_Node->handle_type)
                      .visual_script_info;
              return l_VsInfo.category.is_valid()
                         ? Util::String(l_VsInfo.category.c_str())
                         : get_title(p_Graph);
            }

            virtual Util::String
            get_icon(const Graph &p_Graph) const override
            {
              const auto *l_Node = this;
              if (!l_Node || (u64)l_Node->handle_type == 0) {
                return ICON_LC_LIST;
              }
              const Core::Scripting::VisualScriptTypeInfo &l_VsInfo =
                  Core::Scripting::find_registered_enum_checked(
                      l_Node->handle_type)
                      .visual_script_info;
              return l_VsInfo.icon_name.is_valid()
                         ? get_icon_by_name(l_VsInfo.icon_name)
                         : ICON_LC_LIST;
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_EnumColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {
              const auto *l_Node = this;
              if (!l_Node || (u64)l_Node->handle_type == 0) {
                return;
              }

              Pin l_ValueMetadata =
                  make_enum_pin(g_ValuePinName, l_Node->handle_type);
              l_ValueMetadata.connectable = false;
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_ValueMetadata),
                  p_Schema);

              Pin l_OutputMetadata =
                  make_enum_pin("", l_Node->handle_type);
              l_OutputMetadata.show_default_value_when_unlinked =
                  false;
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_OutputMetadata),
                  p_Schema);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              const Pin *l_ValuePin =
                  p_Graph.find_input_pin_checked(id, g_ValuePinName);
              LOW_ASSERT(
                  l_ValuePin,
                  "Enum literal node is missing its value pin");
              if (!l_ValuePin) {
                return;
              }

              p_Graph.compile_input_pin(l_ValuePin->id,
                                        p_CompileContext);
            }
          };

        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          const Util::Name l_EnumLiteralNodeType =
              p_Graph.register_node_type<EnumLiteralNode>();

          for (const Core::Scripting::EnumInfo &i_Enum :
               Core::Scripting::get_registered_enums()) {
            if (!i_Enum.visual_script_info.exposed) {
              continue;
            }

            const Util::String l_EnumFriendlyName =
                prettify_name(i_Enum.bind_name);
            const Util::String l_Category =
                i_Enum.visual_script_info.category.is_valid()
                    ? Util::String(
                          i_Enum.visual_script_info.category.c_str())
                    : l_EnumFriendlyName;

            NodeSpawnEntry i_SpawnEntry;

            Util::String i_EntryId = "vs_spawn_enum_literal_";
            i_EntryId += ((Util::String)i_Enum.identifier).c_str();

            i_SpawnEntry.id = LOW_NAME(i_EntryId.c_str());
            i_SpawnEntry.category = l_Category;
            i_SpawnEntry.title = l_EnumFriendlyName;
            i_SpawnEntry.subtitle = "Enum";
            i_SpawnEntry.search_text = l_EnumFriendlyName;
            i_SpawnEntry.node_type = l_EnumLiteralNodeType;

            const Util::TypeIdentifier l_Identifier =
                i_Enum.identifier;
            i_SpawnEntry.initialize_node =
                [l_Identifier](Graph &, Node &p_Node) {
                  static_cast<TypedNode &>(p_Node).handle_type =
                      l_Identifier;
                };

            p_Graph.register_spawn_entry(i_SpawnEntry);
          }
        }
      } // namespace EnumNodes
    }   // namespace VisualScript
  }     // namespace Editor
} // namespace Low
