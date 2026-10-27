#include "LowEditorVisualScriptNodes.h"

#include "LowEditorIcons.h"

#include "LowEditor.h"
#include "LowEditorMetadata.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace HandleNodes {
        namespace {
          static ImU32 g_HandleColor = IM_COL32(76, 131, 195, 255);

          static const TypeMetadata *
          get_selected_type(const Graph &p_Graph, NodeId p_NodeId)
          {
            const auto *l_Node =
                p_Graph.find_node_as<TypedNode>(p_NodeId);
            if (!l_Node || !Util::Handle::is_registered_type(
                               l_Node->handle_type)) {
              return nullptr;
            }

            return &get_type_metadata(
                Util::Handle::type_id(l_Node->handle_type));
          }

          static PropertyMetadataBase
          get_selected_property(const Graph &p_Graph, NodeId p_NodeId)
          {
            const TypeMetadata *l_Type =
                get_selected_type(p_Graph, p_NodeId);
            const auto *l_Node =
                p_Graph.find_node_as<TypedNode>(p_NodeId);
            LOW_ASSERT(
                l_Type && l_Node,
                "Handle property node is missing type metadata");
            if (!l_Type || !l_Node) {
              return PropertyMetadataBase{};
            }

            for (const PropertyMetadata &i_Property :
                 l_Type->properties) {
              if (i_Property.name == l_Node->member_name) {
                return i_Property;
              }
            }

            for (const VirtualPropertyMetadata &i_Property :
                 l_Type->virtualProperties) {
              if (i_Property.name == l_Node->member_name) {
                return i_Property;
              }
            }

            auto l_RttiPropertyIt =
                l_Type->typeInfo.properties.find(l_Node->member_name);
            if (l_RttiPropertyIt !=
                l_Type->typeInfo.properties.end()) {
              PropertyMetadataBase l_Metadata;
              l_Metadata.name = l_Node->member_name;
              l_Metadata.friendlyName =
                  prettify_name(l_Node->member_name);
              l_Metadata.editor = false;
              l_Metadata.multiline = false;
              l_Metadata.enumType = false;
              l_Metadata.scriptingExpose = true;
              l_Metadata.hideFlode = false;
              l_Metadata.hideGetterFlode = false;
              l_Metadata.hideSetterFlode = false;
              l_Metadata.getterName = "get_";
              l_Metadata.getterName += l_Metadata.name.c_str();
              l_Metadata.setterName = "set_";
              l_Metadata.setterName += l_Metadata.name.c_str();
              l_Metadata.propInfoBase = l_RttiPropertyIt->second;
              return l_Metadata;
            }

            LOW_ASSERT(false, "Handle property node could not "
                              "resolve selected property");
            return PropertyMetadataBase{};
          }

          static const FunctionMetadata *
          get_selected_function(const Graph &p_Graph, NodeId p_NodeId)
          {
            const TypeMetadata *l_Type =
                get_selected_type(p_Graph, p_NodeId);
            const auto *l_Node =
                p_Graph.find_node_as<TypedNode>(p_NodeId);
            if (!l_Type || !l_Node) {
              return nullptr;
            }

            for (const FunctionMetadata &i_Function :
                 l_Type->functions) {
              if (i_Function.name == l_Node->member_name) {
                return &i_Function;
              }
            }

            return nullptr;
          }

          static const Util::String &
          get_script_type_string(const TypeMetadata &p_Type)
          {
            if (!p_Type.fullScriptingTypeString.empty()) {
              return p_Type.fullScriptingTypeString;
            }
            return p_Type.fullTypeString;
          }

          static Pin make_pin_metadata_from_rtti(
              Util::String p_DisplayName, u32 p_Type,
              Util::TypeIdentifier p_HandleType = {})
          {
            switch (p_Type) {
            case Util::RTTI::PropertyType::BOOL:
              return make_bool_pin(p_DisplayName);
            case Util::RTTI::PropertyType::FLOAT:
              return make_number_pin(p_DisplayName,
                                     NumberSubtype::Float);
            case Util::RTTI::PropertyType::INT:
              return make_number_pin(p_DisplayName,
                                     NumberSubtype::Int32);
            case Util::RTTI::PropertyType::UINT8:
            case Util::RTTI::PropertyType::UINT16:
            case Util::RTTI::PropertyType::UINT32:
            case Util::RTTI::PropertyType::ENUM:
              return make_number_pin(p_DisplayName,
                                     NumberSubtype::UInt32);
            case Util::RTTI::PropertyType::UINT64:
              return make_number_pin(p_DisplayName,
                                     NumberSubtype::UInt64);
            case Util::RTTI::PropertyType::NAME:
              return make_string_pin(p_DisplayName,
                                     StringSubtype::Name);
            case Util::RTTI::PropertyType::STRING:
              return make_string_pin(p_DisplayName,
                                     StringSubtype::String);
            case Util::RTTI::PropertyType::VECTOR2:
              return make_vector2_pin(p_DisplayName);
            case Util::RTTI::PropertyType::VECTOR3:
            case Util::RTTI::PropertyType::COLORRGB: {
              Pin l_Pin;
              l_Pin.display_name = p_DisplayName;
              l_Pin.type = PinType::Vector3;
              l_Pin.default_value =
                  Util::Variant(Math::Vector3(0.0f));
              return l_Pin;
            }
            case Util::RTTI::PropertyType::QUATERNION: {
              Pin l_Pin;
              l_Pin.display_name = p_DisplayName;
              l_Pin.type = PinType::Quaternion;
              l_Pin.default_value = Util::Variant(Math::Quaternion());
              return l_Pin;
            }
            case Util::RTTI::PropertyType::COLOR: {
              Pin l_Pin;
              l_Pin.display_name = p_DisplayName;
              l_Pin.type = PinType::Vector4;
              l_Pin.default_value =
                  Util::Variant(Math::Vector4(0.0f));
              return l_Pin;
            }
            case Util::RTTI::PropertyType::HANDLE:
              return make_handle_pin(p_DisplayName, p_HandleType);
            default:
              return make_dynamic_pin(p_DisplayName);
            }
          }

          static Pin
          make_pin_metadata_from_rtti(Util::String p_DisplayName,
                                      u32 p_Type, u16 p_HandleType)
          {
            return make_pin_metadata_from_rtti(
                p_DisplayName, p_Type,
                p_HandleType ? Util::Handle::identifier(p_HandleType)
                             : Util::TypeIdentifier());
          }

          struct TypeIdNode : public TypedNode
          {
            Util::String get_icon(const Graph &) const override
            {
              return ICON_CI_SYMBOL_CLASS;
            }

            virtual Util::Name get_name() const override
            {
              return N(vs_handle_type_id);
            }

            virtual Util::String
            get_title(const Graph &p_Graph) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              return l_Type ? l_Type->friendlyName + " type id"
                            : Util::String("Type ID");
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_HandleColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {

              Pin l_Metadata =
                  make_number_pin("", NumberSubtype::UInt32);
              l_Metadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_Metadata), p_Schema);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              LOW_ASSERT(l_Type,
                         "Missing selected type for type id node");
              if (!l_Type) {
                return;
              }
              p_CompileContext.main_code.append(
                  get_script_type_string(*l_Type).c_str());
              p_CompileContext.main_code.append("::TYPE_ID");
            }
          };

          struct InstanceCountNode : public TypedNode
          {
            Util::String get_icon(const Graph &) const override
            {
              return ICON_LC_HASH;
            }

            virtual Util::Name get_name() const override
            {
              return N(vs_handle_instance_count);
            }

            virtual Util::String
            get_title(const Graph &p_Graph) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              return l_Type ? l_Type->friendlyName + " instance count"
                            : Util::String("Instance count");
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_HandleColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {

              Pin l_Metadata =
                  make_number_pin("", NumberSubtype::UInt32);
              l_Metadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_Metadata), p_Schema);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              LOW_ASSERT(
                  l_Type,
                  "Missing selected type for instance count node");
              if (!l_Type) {
                return;
              }
              p_CompileContext.main_code.append(
                  get_script_type_string(*l_Type).c_str());
              p_CompileContext.main_code.append("::living_count()");
            }
          };

          struct GetInstanceByIndexNode : public TypedNode
          {
            Util::String get_icon(const Graph &) const override
            {
              return ICON_LC_LIST_ORDERED;
            }

            virtual Util::Name get_name() const override
            {
              return N(vs_handle_get_instance_by_index);
            }

            virtual Util::String
            get_title(const Graph &) const override
            {
              return "Get instance by index";
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_HandleColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              if (!l_Type) {
                return;
              }

              Pin l_IndexMetadata =
                  make_number_pin("Index", NumberSubtype::UInt32);
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_IndexMetadata),
                  p_Schema);

              Pin l_OutputMetadata =
                  make_handle_pin("", l_Type->identifier);
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
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              const Pin *l_IndexPin =
                  p_Graph.find_input_pin_checked(id, "Index");
              LOW_ASSERT(
                  l_Type,
                  "Missing selected type for get instance node");
              if (!l_Type) {
                return;
              }

              p_CompileContext.main_code.append(
                  get_script_type_string(*l_Type).c_str());
              p_CompileContext.main_code.append(
                  "::living_instances()[");
              compile_input_pin(p_Graph, l_IndexPin->id,
                                p_CompileContext);
              p_CompileContext.main_code.append("]");
            }
          };

          struct FindByNameNode : public TypedNode
          {
            Util::String get_icon(const Graph &) const override
            {
              return LOW_EDITOR_ICON_SEARCH;
            }

            virtual Util::Name get_name() const override
            {
              return N(vs_handle_find_by_name);
            }

            virtual Util::String
            get_title(const Graph &) const override
            {
              return "Find by name";
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_HandleColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              if (!l_Type) {
                return;
              }

              Pin l_NameMetadata =
                  make_string_pin("Name", StringSubtype::Name);
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_NameMetadata),
                  p_Schema);

              Pin l_OutputMetadata =
                  make_handle_pin("", l_Type->identifier);
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
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              const Pin *l_NamePin =
                  p_Graph.find_input_pin_checked(id, "Name");
              LOW_ASSERT(
                  l_Type,
                  "Missing selected type for find by name node");
              if (!l_Type) {
                return;
              }

              p_CompileContext.main_code.append(
                  get_script_type_string(*l_Type).c_str());
              p_CompileContext.main_code.append("::find_by_name(");
              compile_input_pin(p_Graph, l_NamePin->id,
                                p_CompileContext);
              p_CompileContext.main_code.append(")");
            }
          };

          struct DestroyNode : public TypedNode
          {
            Util::String get_icon(const Graph &) const override
            {
              return LOW_EDITOR_ICON_DELETE;
            }

            virtual Util::Name get_name() const override
            {
              return N(vs_handle_destroy);
            }

            virtual Util::String
            get_title(const Graph &) const override
            {
              return "Destroy";
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_HandleColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              if (!l_Type) {
                return;
              }

              p_Graph.add_pin(
                  make_input_pin(p_Graph, id,
                                 make_execution_pin("Exec")),
                  p_Schema);

              p_Graph.add_pin(
                  make_output_pin(p_Graph, id,
                                  make_execution_pin("Then")),
                  p_Schema);

              p_Graph.add_pin(
                  make_input_pin(p_Graph, id,
                                 make_handle_pin(l_Type->friendlyName,
                                                 l_Type->identifier)),
                  p_Schema);
            }

            virtual void
            compile(Graph &p_Graph,
                    CompileContext &p_CompileContext) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              const Pin *l_HandlePin = p_Graph.find_input_pin_checked(
                  id, l_Type ? l_Type->friendlyName : "");
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(id, "Then");

              compile_input_pin(p_Graph, l_HandlePin->id,
                                p_CompileContext);
              p_CompileContext.main_code.append(".destroy();").endl();
              p_Graph.continue_compilation(l_ThenPin->id,
                                           p_CompileContext);
            }
          };

          struct GetPropertyNode : public TypedNode
          {
            Util::String get_icon(const Graph &) const override
            {
              return ICON_CI_SYMBOL_PROPERTY;
            }

            virtual Util::Name get_name() const override
            {
              return N(vs_handle_get_property);
            }

            virtual Util::String
            get_title(const Graph &p_Graph) const override
            {
              PropertyMetadataBase l_Property =
                  get_selected_property(p_Graph, id);
              return Util::String("Get ") + l_Property.friendlyName;
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_HandleColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              if (!l_Type) {
                return;
              }
              PropertyMetadataBase l_Property =
                  get_selected_property(p_Graph, id);

              p_Graph.add_pin(
                  make_input_pin(p_Graph, id,
                                 make_handle_pin(l_Type->friendlyName,
                                                 l_Type->identifier)),
                  p_Schema);

              Pin l_OutputMetadata = make_pin_metadata_from_rtti(
                  "", l_Property.propInfoBase.type,
                  l_Property.propInfoBase.handleType);
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
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              PropertyMetadataBase l_Property =
                  get_selected_property(p_Graph, id);
              const Pin *l_HandlePin = p_Graph.find_input_pin_checked(
                  id, l_Type ? l_Type->friendlyName : "");

              compile_input_pin(p_Graph, l_HandlePin->id,
                                p_CompileContext);
              p_CompileContext.main_code.append(".");
              p_CompileContext.main_code.append(
                  l_Property.getterName.c_str());
              p_CompileContext.main_code.append("()");
            }
          };

          struct SetPropertyNode : public TypedNode
          {
            Util::String get_icon(const Graph &) const override
            {
              return ICON_LC_SQUARE_PEN;
            }

            virtual Util::Name get_name() const override
            {
              return N(vs_handle_set_property);
            }

            virtual Util::String
            get_title(const Graph &p_Graph) const override
            {
              PropertyMetadataBase l_Property =
                  get_selected_property(p_Graph, id);
              return Util::String("Set ") + l_Property.friendlyName;
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_HandleColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              if (!l_Type) {
                return;
              }
              PropertyMetadataBase l_Property =
                  get_selected_property(p_Graph, id);

              p_Graph.add_pin(
                  make_input_pin(p_Graph, id,
                                 make_execution_pin("Exec")),
                  p_Schema);

              p_Graph.add_pin(
                  make_output_pin(p_Graph, id,
                                  make_execution_pin("Then")),
                  p_Schema);

              p_Graph.add_pin(
                  make_input_pin(p_Graph, id,
                                 make_handle_pin(l_Type->friendlyName,
                                                 l_Type->identifier)),
                  p_Schema);

              Pin l_ValueMetadata = make_pin_metadata_from_rtti(
                  l_Property.friendlyName,
                  l_Property.propInfoBase.type,
                  l_Property.propInfoBase.handleType);
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_ValueMetadata),
                  p_Schema);
            }

            virtual void
            compile(Graph &p_Graph,
                    CompileContext &p_CompileContext) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              PropertyMetadataBase l_Property =
                  get_selected_property(p_Graph, id);
              const Pin *l_HandlePin = p_Graph.find_input_pin_checked(
                  id, l_Type ? l_Type->friendlyName : "");
              const Pin *l_ValuePin = p_Graph.find_input_pin_checked(
                  id, l_Property.friendlyName);
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(id, "Then");

              compile_input_pin(p_Graph, l_HandlePin->id,
                                p_CompileContext);
              p_CompileContext.main_code.append(".");
              p_CompileContext.main_code.append(
                  l_Property.setterName.c_str());
              p_CompileContext.main_code.append("(");
              compile_input_pin(p_Graph, l_ValuePin->id,
                                p_CompileContext);
              p_CompileContext.main_code.append(");").endl();

              p_Graph.continue_compilation(l_ThenPin->id,
                                           p_CompileContext);
            }
          };

          struct FunctionNode : public TypedNode
          {
            Util::String get_icon(const Graph &) const override
            {
              return ICON_CI_SYMBOL_METHOD;
            }

            virtual Util::Name get_name() const override
            {
              return N(vs_handle_function);
            }

            virtual Util::String
            get_title(const Graph &p_Graph) const override
            {
              const FunctionMetadata *l_Function =
                  get_selected_function(p_Graph, id);
              return l_Function ? l_Function->friendlyName
                                : Util::String("Function");
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_HandleColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              const FunctionMetadata *l_Function =
                  get_selected_function(p_Graph, id);
              if (!l_Type || !l_Function) {
                return;
              }

              if (l_Function->hasReturnValue) {

                Pin l_ResultMetadata = make_pin_metadata_from_rtti(
                    "", l_Function->functionInfo.type,
                    l_Function->functionInfo.handleType);
                l_ResultMetadata.show_default_value_when_unlinked =
                    false;
                p_Graph.add_pin(
                    make_output_pin(p_Graph, id, l_ResultMetadata),
                    p_Schema);
              } else {

                p_Graph.add_pin(
                    make_input_pin(p_Graph, id,
                                   make_execution_pin("Exec")),
                    p_Schema);

                p_Graph.add_pin(
                    make_output_pin(p_Graph, id,
                                    make_execution_pin("Then")),
                    p_Schema);
              }

              if (!l_Function->isStatic) {

                p_Graph.add_pin(
                    make_input_pin(
                        p_Graph, id,
                        make_handle_pin(l_Type->friendlyName,
                                        l_Type->identifier)),
                    p_Schema);
              }

              for (const ParameterMetadata &i_Param :
                   l_Function->parameters) {

                Pin l_ParamMetadata = make_pin_metadata_from_rtti(
                    i_Param.friendlyName, i_Param.paramInfo.type,
                    i_Param.paramInfo.handleType);
                p_Graph.add_pin(
                    make_input_pin(p_Graph, id, l_ParamMetadata),
                    p_Schema);
              }
            }

            void append_call(Graph &p_Graph, NodeId id,
                             CompileContext &p_CompileContext) const
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              const FunctionMetadata *l_Function =
                  get_selected_function(p_Graph, id);
              LOW_ASSERT(l_Type && l_Function,
                         "Missing function metadata for handle "
                         "function node");
              if (!l_Type || !l_Function) {
                return;
              }

              u32 l_InputOffset = l_Function->hasReturnValue ? 0 : 2;

              if (l_Function->isStatic) {
                p_CompileContext.main_code.append(
                    get_script_type_string(*l_Type).c_str());
                p_CompileContext.main_code.append("::");
              } else {
                const Pin *l_InstancePin =
                    p_Graph.find_input_pin_checked(
                        id, l_Type->friendlyName);
                compile_input_pin(p_Graph, l_InstancePin->id,
                                  p_CompileContext);
                p_CompileContext.main_code.append(".");
                l_InputOffset += 1;
              }

              p_CompileContext.main_code.append(
                  l_Function->name.c_str());
              p_CompileContext.main_code.append("(");

              for (u32 i = 0; i < l_Function->parameters.size();
                   ++i) {
                if (i) {
                  p_CompileContext.main_code.append(", ");
                }
                const Pin *l_ParamPin =
                    p_Graph.find_input_pin_checked(
                        id, l_Function->parameters[i].friendlyName);
                compile_input_pin(p_Graph, l_ParamPin->id,
                                  p_CompileContext);
              }

              p_CompileContext.main_code.append(")");
            }

            virtual void
            compile(Graph &p_Graph,
                    CompileContext &p_CompileContext) const override
            {
              const FunctionMetadata *l_Function =
                  get_selected_function(p_Graph, id);
              LOW_ASSERT(
                  l_Function && !l_Function->hasReturnValue,
                  "Tried to statement-compile returning handle "
                  "function node");
              if (!l_Function || l_Function->hasReturnValue) {
                return;
              }

              append_call(p_Graph, id, p_CompileContext);
              p_CompileContext.main_code.append(";").endl();

              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(id, "Then");
              p_Graph.continue_compilation(l_ThenPin->id,
                                           p_CompileContext);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              const FunctionMetadata *l_Function =
                  get_selected_function(p_Graph, id);
              LOW_ASSERT(l_Function && l_Function->hasReturnValue,
                         "Tried to expression-compile void handle "
                         "function node");
              if (!l_Function || !l_Function->hasReturnValue) {
                return;
              }

              append_call(p_Graph, id, p_CompileContext);
            }
          };

          struct ForEachInstanceNode : public TypedNode
          {
            Util::String get_icon(const Graph &) const override
            {
              return ICON_LC_REPEAT;
            }

            virtual Util::Name get_name() const override
            {
              return N(vs_handle_for_each_instance);
            }

            virtual Util::String
            get_title(const Graph &) const override
            {
              return "For each instance";
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_HandleColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              if (!l_Type) {
                return;
              }

              p_Graph.add_pin(
                  make_input_pin(p_Graph, id,
                                 make_execution_pin("Exec")),
                  p_Schema);

              p_Graph.add_pin(
                  make_output_pin(p_Graph, id,
                                  make_execution_pin("Then")),
                  p_Schema);

              p_Graph.add_pin(
                  make_output_pin(p_Graph, id,
                                  make_execution_pin("Loop")),
                  p_Schema);

              Pin l_InstanceMetadata =
                  make_handle_pin("Instance", l_Type->identifier);
              l_InstanceMetadata.show_default_value_when_unlinked =
                  false;
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_InstanceMetadata),
                  p_Schema);
            }

            virtual void
            compile(Graph &p_Graph,
                    CompileContext &p_CompileContext) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, id);
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(id, "Then");
              const Pin *l_LoopPin =
                  p_Graph.find_output_pin_checked(id, "Loop");
              LOW_ASSERT(l_Type,
                         "Missing selected type for foreach node");
              if (!l_Type) {
                return;
              }

              Util::StringBuilder l_IndexVariableName;
              l_IndexVariableName.append("__foreach_index")
                  .append((u64)id.value);
              Util::StringBuilder l_InstanceVariableName;
              l_InstanceVariableName.append("__foreach_instance")
                  .append((u64)id.value);

              p_CompileContext.main_code.append("for (int ")
                  .append(l_IndexVariableName.get())
                  .append(" = 0; ")
                  .append(l_IndexVariableName.get())
                  .append(" < ")
                  .append(get_script_type_string(*l_Type).c_str())
                  .append("::living_count(); ++")
                  .append(l_IndexVariableName.get())
                  .append(") {")
                  .endl();
              p_CompileContext.main_code
                  .append(get_script_type_string(*l_Type).c_str())
                  .append(" ")
                  .append(l_InstanceVariableName.get())
                  .append(" = ")
                  .append(get_script_type_string(*l_Type).c_str())
                  .append("::living_instances()[")
                  .append(l_IndexVariableName.get())
                  .append("];")
                  .endl();
              p_Graph.continue_compilation(l_LoopPin->id,
                                           p_CompileContext);
              p_CompileContext.main_code.append("}").endl();

              p_Graph.continue_compilation(l_ThenPin->id,
                                           p_CompileContext);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_Graph;
              (void)p_PinId;
              Util::StringBuilder l_InstanceVariableName;
              l_InstanceVariableName.append("__foreach_instance")
                  .append((u64)id.value);
              p_CompileContext.main_code.append(
                  l_InstanceVariableName.get());
            }
          };

          static void register_spawn_entry(
              Graph &p_Graph, Util::String p_Category,
              Util::String p_Title, Util::String p_Subtitle,
              Util::String p_SearchText, Util::Name p_NodeType,
              Util::TypeIdentifier p_HandleType,
              Util::Name p_MemberName = {})
          {
            NodeSpawnEntry l_Entry;
            Util::String l_IdString = "vs_spawn_handle_";
            l_IdString += p_NodeType.c_str();
            l_IdString += "_";
            l_IdString += ((Util::String)p_HandleType).c_str();
            if (p_MemberName.is_valid()) {
              l_IdString += "_";
              l_IdString += p_MemberName.c_str();
            }
            l_Entry.id = LOW_NAME(l_IdString.c_str());
            l_Entry.category = p_Category;
            l_Entry.title = p_Title;
            l_Entry.subtitle = p_Subtitle;
            l_Entry.search_text = p_SearchText;
            l_Entry.node_type = p_NodeType;
            l_Entry.initialize_node =
                [p_HandleType, p_MemberName](Graph &, Node &p_Node) {
                  static_cast<TypedNode &>(p_Node).handle_type =
                      p_HandleType;
                  static_cast<TypedNode &>(p_Node).member_name =
                      p_MemberName;
                };
            p_Graph.register_spawn_entry(l_Entry);
          }
        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          const Util::Name l_TypeIdNodeType =
              p_Graph.register_node_type<TypeIdNode>();
          const Util::Name l_InstanceCountNodeType =
              p_Graph.register_node_type<InstanceCountNode>();
          const Util::Name l_GetInstanceByIndexNodeType =
              p_Graph.register_node_type<GetInstanceByIndexNode>();
          const Util::Name l_FindByNameNodeType =
              p_Graph.register_node_type<FindByNameNode>();
          const Util::Name l_DestroyNodeType =
              p_Graph.register_node_type<DestroyNode>();
          const Util::Name l_GetPropertyNodeType =
              p_Graph.register_node_type<GetPropertyNode>();
          const Util::Name l_SetPropertyNodeType =
              p_Graph.register_node_type<SetPropertyNode>();
          const Util::Name l_FunctionNodeType =
              p_Graph.register_node_type<FunctionNode>();
          const Util::Name l_ForEachInstanceNodeType =
              p_Graph.register_node_type<ForEachInstanceNode>();

          for (auto &it : get_type_metadata()) {
            const TypeMetadata &i_Type = it.second;
            if (!i_Type.scriptingExpose) {
              continue;
            }

            register_spawn_entry(p_Graph, i_Type.friendlyName,
                                 "Type ID", i_Type.friendlyName,
                                 Util::String("type id ") +
                                     i_Type.friendlyName,
                                 l_TypeIdNodeType, i_Type.identifier);
            register_spawn_entry(
                p_Graph, i_Type.friendlyName, "Instance count",
                i_Type.friendlyName,
                Util::String("instance count ") + i_Type.friendlyName,
                l_InstanceCountNodeType, i_Type.identifier);
            register_spawn_entry(
                p_Graph, i_Type.friendlyName,
                "Get living instance by index", i_Type.friendlyName,
                Util::String("get living instance index ") +
                    i_Type.friendlyName,
                l_GetInstanceByIndexNodeType, i_Type.identifier);
            register_spawn_entry(
                p_Graph, i_Type.friendlyName, "For each instance",
                i_Type.friendlyName,
                Util::String("for each instance ") +
                    i_Type.friendlyName,
                l_ForEachInstanceNodeType, i_Type.identifier);
            register_spawn_entry(
                p_Graph, i_Type.friendlyName, "Find by name",
                i_Type.friendlyName,
                Util::String("find by name ") + i_Type.friendlyName,
                l_FindByNameNodeType, i_Type.identifier);
            register_spawn_entry(
                p_Graph, i_Type.friendlyName, "Destroy",
                i_Type.friendlyName,
                Util::String("destroy ") + i_Type.friendlyName,
                l_DestroyNodeType, i_Type.identifier);

            for (const PropertyMetadata &i_Property :
                 i_Type.properties) {
              if (!i_Property.scriptingExpose ||
                  i_Property.hideFlode) {
                continue;
              }

              if (!i_Property.hideGetterFlode) {
                register_spawn_entry(
                    p_Graph, i_Type.friendlyName,
                    Util::String("Get ") + i_Property.friendlyName,
                    i_Type.friendlyName,
                    Util::String("get ") + i_Property.friendlyName +
                        " " + i_Type.friendlyName,
                    l_GetPropertyNodeType, i_Type.identifier,
                    i_Property.name);
              }

              if (!i_Property.hideSetterFlode) {
                register_spawn_entry(
                    p_Graph, i_Type.friendlyName,
                    Util::String("Set ") + i_Property.friendlyName,
                    i_Type.friendlyName,
                    Util::String("set ") + i_Property.friendlyName +
                        " " + i_Type.friendlyName,
                    l_SetPropertyNodeType, i_Type.identifier,
                    i_Property.name);
              }
            }

            for (const VirtualPropertyMetadata &i_Property :
                 i_Type.virtualProperties) {
              if (!i_Property.scriptingExpose) {
                continue;
              }

              register_spawn_entry(
                  p_Graph, i_Type.friendlyName,
                  Util::String("Get ") + i_Property.friendlyName,
                  i_Type.friendlyName,
                  Util::String("get ") + i_Property.friendlyName +
                      " " + i_Type.friendlyName,
                  l_GetPropertyNodeType, i_Type.identifier,
                  i_Property.name);
              register_spawn_entry(
                  p_Graph, i_Type.friendlyName,
                  Util::String("Set ") + i_Property.friendlyName,
                  i_Type.friendlyName,
                  Util::String("set ") + i_Property.friendlyName +
                      " " + i_Type.friendlyName,
                  l_SetPropertyNodeType, i_Type.identifier,
                  i_Property.name);
            }

            for (const FunctionMetadata &i_Function :
                 i_Type.functions) {
              if (!i_Function.scriptingExpose ||
                  i_Function.hideFlode) {
                continue;
              }

              register_spawn_entry(
                  p_Graph, i_Type.friendlyName,
                  i_Function.friendlyName, i_Type.friendlyName,
                  i_Function.friendlyName + " " + i_Type.friendlyName,
                  l_FunctionNodeType, i_Type.identifier,
                  i_Function.name);
            }
          }
        }
      } // namespace HandleNodes
    }   // namespace VisualScript
  }     // namespace Editor
} // namespace Low
