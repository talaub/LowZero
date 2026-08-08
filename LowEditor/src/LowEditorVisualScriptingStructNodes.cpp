#include "LowCoreScripting.h"
#include "LowEditor.h"
#include "LowEditorVisualScriptNodes.h"

#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace StructNodes {
        namespace {
          static ImU32 g_StructColor = IM_COL32(189, 128, 53, 255);

          static bool make_pin_metadata_from_struct_field(
              const Util::RTTI::StructFieldInfo &p_Field, Pin &p_Pin)
          {
            const PinContainerType l_Container =
                p_Field.container == Util::RTTI::ContainerType::LIST
                    ? PinContainerType::List
                    : PinContainerType::None;

            switch (p_Field.type) {
            case Util::RTTI::PropertyType::BOOL:
              p_Pin = make_bool_pin_metadata(p_Field.name.c_str());
              p_Pin.container_type = l_Container;
              return true;
            case Util::RTTI::PropertyType::FLOAT:
              p_Pin = make_number_pin_metadata(p_Field.name.c_str(),
                                              NumberSubtype::Float,
                                              l_Container);
              return true;
            case Util::RTTI::PropertyType::INT:
              p_Pin = make_number_pin_metadata(p_Field.name.c_str(),
                                              NumberSubtype::Int32,
                                              l_Container);
              return true;
            case Util::RTTI::PropertyType::UINT8:
            case Util::RTTI::PropertyType::UINT16:
            case Util::RTTI::PropertyType::UINT32:
              p_Pin = make_number_pin_metadata(p_Field.name.c_str(),
                                              NumberSubtype::UInt32,
                                              l_Container);
              return true;
            case Util::RTTI::PropertyType::UINT64:
              p_Pin = make_number_pin_metadata(p_Field.name.c_str(),
                                              NumberSubtype::UInt64,
                                              l_Container);
              return true;
            case Util::RTTI::PropertyType::NAME:
              p_Pin = make_string_pin_metadata(p_Field.name.c_str(),
                                              StringSubtype::Name,
                                              l_Container);
              return true;
            case Util::RTTI::PropertyType::STRING:
              p_Pin = make_string_pin_metadata(p_Field.name.c_str(),
                                              StringSubtype::String,
                                              l_Container);
              return true;
            case Util::RTTI::PropertyType::VECTOR2:
              p_Pin = make_vector2_pin_metadata(p_Field.name.c_str(),
                                                l_Container);
              return true;
            case Util::RTTI::PropertyType::VECTOR3:
            case Util::RTTI::PropertyType::COLORRGB:
              p_Pin = make_vector3_pin_metadata(p_Field.name.c_str(),
                                                l_Container);
              return true;
            case Util::RTTI::PropertyType::VECTOR4:
            case Util::RTTI::PropertyType::COLOR: {
              Pin l_Pin;
              l_Pin.display_name = p_Field.name.c_str();
              l_Pin.type = PinType::Vector4;
              l_Pin.container_type = l_Container;
              l_Pin.default_value =
                  Util::Variant(Math::Vector4(0.0f));
              p_Pin = l_Pin;
              return true;
            }
            case Util::RTTI::PropertyType::QUATERNION: {
              Pin l_Pin;
              l_Pin.display_name = p_Field.name.c_str();
              l_Pin.type = PinType::Quaternion;
              l_Pin.container_type = l_Container;
              l_Pin.default_value = Util::Variant(Math::Quaternion());
              p_Pin = l_Pin;
              return true;
            }
            case Util::RTTI::PropertyType::HANDLE:
              p_Pin = make_handle_pin_metadata(p_Field.name.c_str(),
                                              p_Field.referenced_type,
                                              l_Container);
              return true;
            case Util::RTTI::PropertyType::ENUM:
              p_Pin = make_enum_pin_metadata(p_Field.name.c_str(),
                                            p_Field.referenced_type,
                                            l_Container);
              return true;
            case Util::RTTI::PropertyType::STRUCT:
              p_Pin = make_struct_pin_metadata(p_Field.name.c_str(),
                                              p_Field.referenced_type,
                                              l_Container);
              return true;
            default:
              // Covers UNKNOWN and VOID - Break Struct simply does not
              // expose these fields.
              return false;
            }
          }

          struct BreakStructNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_break_struct);
            }

            virtual Util::String
            get_title(const Graph &p_Graph,
                      NodeId p_NodeId) const override
            {
              const Node *l_Node = p_Graph.find_node(p_NodeId);
              if (!l_Node || (u64)l_Node->handle_type == 0) {
                return "Break Struct";
              }
              Util::String l_Title = "Break ";
              l_Title += Core::Scripting::find_registered_struct_checked(
                             l_Node->handle_type)
                             .bind_name.c_str();
              return l_Title;
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph,
                        NodeId p_NodeId) const override
            {
              const Node *l_Node = p_Graph.find_node(p_NodeId);
              if (!l_Node || (u64)l_Node->handle_type == 0) {
                return "Struct";
              }
              return prettify_name(
                  Core::Scripting::find_registered_struct_checked(
                      l_Node->handle_type)
                      .bind_name);
            }

            virtual Util::String
            get_category(const Graph &p_Graph,
                        NodeId p_NodeId) const override
            {
              const Node *l_Node = p_Graph.find_node(p_NodeId);
              if (!l_Node || (u64)l_Node->handle_type == 0) {
                return "Struct";
              }
              const Core::Scripting::VisualScriptTypeInfo &l_VsInfo =
                  Core::Scripting::find_registered_struct_checked(
                      l_Node->handle_type)
                      .visual_script_info;
              return l_VsInfo.category.is_valid()
                        ? Util::String(l_VsInfo.category.c_str())
                        : get_subtitle(p_Graph, p_NodeId);
            }

            virtual Util::String
            get_icon(const Graph &p_Graph,
                    NodeId p_NodeId) const override
            {
              const Node *l_Node = p_Graph.find_node(p_NodeId);
              if (!l_Node || (u64)l_Node->handle_type == 0) {
                return ICON_LC_LIST_TREE;
              }
              const Core::Scripting::VisualScriptTypeInfo &l_VsInfo =
                  Core::Scripting::find_registered_struct_checked(
                      l_Node->handle_type)
                      .visual_script_info;
              return l_VsInfo.icon_name.is_valid()
                        ? get_icon_by_name(l_VsInfo.icon_name)
                        : ICON_LC_LIST_TREE;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_StructColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              const Node *l_Node = p_Graph.find_node(p_NodeId);
              if (!l_Node || (u64)l_Node->handle_type == 0) {
                return;
              }

              Editor::Pin l_StructIn =
                  make_input_pin(p_Graph, p_NodeId);
              Pin l_StructMetadata =
                  make_struct_pin_metadata("", l_Node->handle_type);
              p_Graph.add_pin(l_StructIn, l_StructMetadata, p_Schema);

              const Util::RTTI::StructInfo &l_StructInfo =
                  Util::get_struct_info(l_Node->handle_type);
              for (const Util::RTTI::StructFieldInfo &i_Field :
                   l_StructInfo.fields) {
                Pin i_FieldPin;
                if (!make_pin_metadata_from_struct_field(i_Field,
                                                         i_FieldPin)) {
                  continue;
                }

                Editor::Pin i_FieldOut =
                    make_output_pin(p_Graph, p_NodeId);
                p_Graph.add_pin(i_FieldOut, i_FieldPin, p_Schema);
              }
            }

            virtual void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              const Pin *l_OutputPin = p_Graph.find_pin(p_PinId);
              LOW_ASSERT(l_OutputPin,
                        "Could not find break struct output pin");
              if (!l_OutputPin) {
                return;
              }

              const Pin *l_StructInputPin =
                  p_Graph.find_input_pin_checked(p_NodeId, "");
              LOW_ASSERT(l_StructInputPin,
                        "Break struct node is missing struct input");
              if (!l_StructInputPin) {
                return;
              }

              p_Graph.compile_input_pin(l_StructInputPin->pin,
                                        p_CompileContext);
              p_CompileContext.main_code.append(".");
              p_CompileContext.main_code.append(
                  l_OutputPin->display_name);
            }
          };

          static BreakStructNodeClass g_BreakStructNodeClass;
        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          p_Graph.register_node_class(g_BreakStructNodeClass);

          for (const Core::Scripting::StructInfo &i_Struct :
               Core::Scripting::get_registered_structs()) {
            if (!i_Struct.visual_script_info.exposed) {
              continue;
            }

            const Util::String l_StructFriendlyName =
                prettify_name(i_Struct.bind_name);
            const Util::String l_Category =
                i_Struct.visual_script_info.category.is_valid()
                    ? Util::String(
                          i_Struct.visual_script_info.category.c_str())
                    : l_StructFriendlyName;

            NodeSpawnEntry i_SpawnEntry;

            Util::String i_EntryId = "vs_spawn_break_struct_";
            i_EntryId += ((Util::String)i_Struct.identifier).c_str();

            i_SpawnEntry.id = LOW_NAME(i_EntryId.c_str());
            i_SpawnEntry.category = l_Category;
            i_SpawnEntry.title =
                Util::String("Break ") + l_StructFriendlyName;
            i_SpawnEntry.subtitle = l_StructFriendlyName;
            i_SpawnEntry.search_text =
                Util::String("break ") + l_StructFriendlyName;
            i_SpawnEntry.node_class = g_BreakStructNodeClass.get_name();

            const Util::TypeIdentifier l_Identifier =
                i_Struct.identifier;
            i_SpawnEntry.initialize_node = [l_Identifier](Graph &,
                                                          Node &p_Node) {
              p_Node.handle_type = l_Identifier;
            };

            p_Graph.register_spawn_entry(i_SpawnEntry);
          }
        }
      } // namespace StructNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
