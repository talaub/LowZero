#include "LowEditorVisualScriptNodes.h"

#include "LowEditor.h"
#include "LowEditorMetadata.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace HandleNodes {
        namespace {
          static ImU32 g_HandleColor = IM_COL32(76, 131, 195, 255);

          static const TypeMetadata *get_selected_type(
              const Graph &p_Graph, NodeId p_NodeId)
          {
            const Node *l_Node = p_Graph.find_node(p_NodeId);
            if (!l_Node ||
                !Util::Handle::is_registered_type(l_Node->handle_type)) {
              return nullptr;
            }

            return &get_type_metadata(
                Util::Handle::type_id(l_Node->handle_type));
          }

          static PropertyMetadataBase get_selected_property(
              const Graph &p_Graph, NodeId p_NodeId)
          {
            const TypeMetadata *l_Type =
                get_selected_type(p_Graph, p_NodeId);
            const Node *l_Node = p_Graph.find_node(p_NodeId);
            LOW_ASSERT(l_Type && l_Node,
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
            if (l_RttiPropertyIt != l_Type->typeInfo.properties.end()) {
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

            LOW_ASSERT(
                false,
                "Handle property node could not resolve selected property");
            return PropertyMetadataBase{};
          }

          static const FunctionMetadata *get_selected_function(
              const Graph &p_Graph, NodeId p_NodeId)
          {
            const TypeMetadata *l_Type =
                get_selected_type(p_Graph, p_NodeId);
            const Node *l_Node = p_Graph.find_node(p_NodeId);
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

          static const Util::String &get_script_type_string(
              const TypeMetadata &p_Type)
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
              return make_bool_pin_metadata(p_DisplayName);
            case Util::RTTI::PropertyType::FLOAT:
              return make_number_pin_metadata(
                  p_DisplayName, NumberSubtype::Float);
            case Util::RTTI::PropertyType::INT:
              return make_number_pin_metadata(
                  p_DisplayName, NumberSubtype::Int32);
            case Util::RTTI::PropertyType::UINT8:
            case Util::RTTI::PropertyType::UINT16:
            case Util::RTTI::PropertyType::UINT32:
            case Util::RTTI::PropertyType::ENUM:
              return make_number_pin_metadata(
                  p_DisplayName, NumberSubtype::UInt32);
            case Util::RTTI::PropertyType::UINT64:
              return make_number_pin_metadata(
                  p_DisplayName, NumberSubtype::UInt64);
            case Util::RTTI::PropertyType::NAME:
              return make_string_pin_metadata(
                  p_DisplayName, StringSubtype::Name);
            case Util::RTTI::PropertyType::STRING:
              return make_string_pin_metadata(
                  p_DisplayName, StringSubtype::String);
            case Util::RTTI::PropertyType::VECTOR2:
              return make_vector2_pin_metadata(p_DisplayName);
            case Util::RTTI::PropertyType::VECTOR3:
            case Util::RTTI::PropertyType::COLORRGB: {
              Pin l_Pin;
              l_Pin.display_name = p_DisplayName;
              l_Pin.type = PinType::Vector3;
              l_Pin.default_value = Util::Variant(Math::Vector3(0.0f));
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
              return make_handle_pin_metadata(p_DisplayName,
                                              p_HandleType);
            default:
              return make_dynamic_pin_metadata(p_DisplayName);
            }
          }

          static Pin make_pin_metadata_from_rtti(
              Util::String p_DisplayName, u32 p_Type,
              u16 p_HandleType)
          {
            return make_pin_metadata_from_rtti(
                p_DisplayName, p_Type,
                p_HandleType
                    ? Util::Handle::identifier(p_HandleType)
                    : Util::TypeIdentifier());
          }

          struct TypeIdNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_handle_type_id);
            }

            virtual Util::String
            get_title(const Graph &p_Graph, NodeId p_NodeId) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              return l_Type ? l_Type->friendlyName + " type id"
                            : Util::String("Type ID");
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph,
                         NodeId p_NodeId) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &, NodeId) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_HandleColor;
            }

            virtual void
            setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                               const NodeGraphSchema *p_Schema) const
                override
            {
              Editor::Pin l_Output =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_Metadata = make_number_pin_metadata(
                  "", NumberSubtype::UInt32);
              l_Metadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(l_Output, l_Metadata, p_Schema);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              LOW_ASSERT(l_Type, "Missing selected type for type id node");
              if (!l_Type) {
                return;
              }
              p_CompileContext.main_code.append(
                  get_script_type_string(*l_Type).c_str());
              p_CompileContext.main_code.append("::TYPE_ID");
            }
          };

          struct InstanceCountNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_handle_instance_count);
            }

            virtual Util::String
            get_title(const Graph &p_Graph, NodeId p_NodeId) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              return l_Type
                         ? l_Type->friendlyName + " instance count"
                         : Util::String("Instance count");
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph,
                         NodeId p_NodeId) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &, NodeId) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_HandleColor;
            }

            virtual void
            setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                               const NodeGraphSchema *p_Schema) const
                override
            {
              Editor::Pin l_Output =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_Metadata = make_number_pin_metadata(
                  "", NumberSubtype::UInt32);
              l_Metadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(l_Output, l_Metadata, p_Schema);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              LOW_ASSERT(l_Type,
                         "Missing selected type for instance count node");
              if (!l_Type) {
                return;
              }
              p_CompileContext.main_code.append(
                  get_script_type_string(*l_Type).c_str());
              p_CompileContext.main_code.append("::living_count()");
            }
          };

          struct GetInstanceByIndexNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_handle_get_instance_by_index);
            }

            virtual Util::String
            get_title(const Graph &, NodeId) const override
            {
              return "Get instance by index";
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph,
                         NodeId p_NodeId) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &, NodeId) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_HandleColor;
            }

            virtual void
            setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                               const NodeGraphSchema *p_Schema) const
                override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              if (!l_Type) {
                return;
              }

              Editor::Pin l_IndexIn = make_input_pin(p_Graph, p_NodeId);
              Pin l_IndexMetadata = make_number_pin_metadata(
                  "Index", NumberSubtype::UInt32);
              p_Graph.add_pin(l_IndexIn, l_IndexMetadata, p_Schema);

              Editor::Pin l_Output =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_OutputMetadata = make_handle_pin_metadata(
                  "", l_Type->identifier);
              l_OutputMetadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(l_Output, l_OutputMetadata, p_Schema);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              const Pin *l_IndexPin =
                  p_Graph.find_input_pin_checked(p_NodeId, "Index");
              LOW_ASSERT(l_Type,
                         "Missing selected type for get instance node");
              if (!l_Type) {
                return;
              }

              p_CompileContext.main_code.append(
                  get_script_type_string(*l_Type).c_str());
              p_CompileContext.main_code.append("::living_instances()[");
              compile_input_pin(p_Graph, p_NodeId, l_IndexPin->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append("]");
            }
          };

          struct FindByNameNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_handle_find_by_name);
            }

            virtual Util::String
            get_title(const Graph &, NodeId) const override
            {
              return "Find by name";
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph,
                         NodeId p_NodeId) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &, NodeId) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_HandleColor;
            }

            virtual void
            setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                               const NodeGraphSchema *p_Schema) const
                override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              if (!l_Type) {
                return;
              }

              Editor::Pin l_NameIn = make_input_pin(p_Graph, p_NodeId);
              Pin l_NameMetadata = make_string_pin_metadata(
                  "Name", StringSubtype::Name);
              p_Graph.add_pin(l_NameIn, l_NameMetadata, p_Schema);

              Editor::Pin l_Output =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_OutputMetadata = make_handle_pin_metadata(
                  "", l_Type->identifier);
              l_OutputMetadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(l_Output, l_OutputMetadata, p_Schema);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              const Pin *l_NamePin =
                  p_Graph.find_input_pin_checked(p_NodeId, "Name");
              LOW_ASSERT(l_Type,
                         "Missing selected type for find by name node");
              if (!l_Type) {
                return;
              }

              p_CompileContext.main_code.append(
                  get_script_type_string(*l_Type).c_str());
              p_CompileContext.main_code.append("::find_by_name(");
              compile_input_pin(p_Graph, p_NodeId, l_NamePin->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(")");
            }
          };

          struct DestroyNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_handle_destroy);
            }

            virtual Util::String
            get_title(const Graph &, NodeId) const override
            {
              return "Destroy";
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph,
                         NodeId p_NodeId) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &, NodeId) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_HandleColor;
            }

            virtual void
            setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                               const NodeGraphSchema *p_Schema) const
                override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              if (!l_Type) {
                return;
              }

              Editor::Pin l_ExecIn = make_input_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(l_ExecIn,
                              make_execution_pin_metadata("Exec"),
                              p_Schema);

              Editor::Pin l_ExecOut =
                  make_output_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(l_ExecOut,
                              make_execution_pin_metadata("Then"),
                              p_Schema);

              Editor::Pin l_HandleIn = make_input_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(l_HandleIn,
                              make_handle_pin_metadata(
                                  l_Type->friendlyName, l_Type->identifier),
                              p_Schema);
            }

            virtual void compile(Graph &p_Graph, NodeId p_NodeId,
                                 CompileContext &p_CompileContext) const
                override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              const Pin *l_HandlePin = p_Graph.find_input_pin_checked(
                  p_NodeId, l_Type ? l_Type->friendlyName : "");
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Then");

              compile_input_pin(p_Graph, p_NodeId, l_HandlePin->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(".destroy();").endl();
              p_Graph.continue_compilation(l_ThenPin->pin,
                                           p_CompileContext);
            }
          };

          struct GetPropertyNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_handle_get_property);
            }

            virtual Util::String
            get_title(const Graph &p_Graph, NodeId p_NodeId) const override
            {
              PropertyMetadataBase l_Property =
                  get_selected_property(p_Graph, p_NodeId);
              return Util::String("Get ") + l_Property.friendlyName;
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph,
                         NodeId p_NodeId) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &, NodeId) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_HandleColor;
            }

            virtual void
            setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                               const NodeGraphSchema *p_Schema) const
                override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              if (!l_Type) {
                return;
              }
              PropertyMetadataBase l_Property =
                  get_selected_property(p_Graph, p_NodeId);

              Editor::Pin l_HandleIn = make_input_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(l_HandleIn,
                              make_handle_pin_metadata(
                                  l_Type->friendlyName, l_Type->identifier),
                              p_Schema);

              Editor::Pin l_Output =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_OutputMetadata = make_pin_metadata_from_rtti(
                  "", l_Property.propInfoBase.type,
                  l_Property.propInfoBase.handleType);
              l_OutputMetadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(l_Output, l_OutputMetadata, p_Schema);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              PropertyMetadataBase l_Property =
                  get_selected_property(p_Graph, p_NodeId);
              const Pin *l_HandlePin = p_Graph.find_input_pin_checked(
                  p_NodeId, l_Type ? l_Type->friendlyName : "");

              compile_input_pin(p_Graph, p_NodeId, l_HandlePin->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(".");
              p_CompileContext.main_code.append(
                  l_Property.getterName.c_str());
              p_CompileContext.main_code.append("()");
            }
          };

          struct SetPropertyNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_handle_set_property);
            }

            virtual Util::String
            get_title(const Graph &p_Graph, NodeId p_NodeId) const override
            {
              PropertyMetadataBase l_Property =
                  get_selected_property(p_Graph, p_NodeId);
              return Util::String("Set ") + l_Property.friendlyName;
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph,
                         NodeId p_NodeId) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &, NodeId) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_HandleColor;
            }

            virtual void
            setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                               const NodeGraphSchema *p_Schema) const
                override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              if (!l_Type) {
                return;
              }
              PropertyMetadataBase l_Property =
                  get_selected_property(p_Graph, p_NodeId);

              Editor::Pin l_ExecIn = make_input_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(l_ExecIn,
                              make_execution_pin_metadata("Exec"),
                              p_Schema);

              Editor::Pin l_ExecOut =
                  make_output_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(l_ExecOut,
                              make_execution_pin_metadata("Then"),
                              p_Schema);

              Editor::Pin l_HandleIn = make_input_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(l_HandleIn,
                              make_handle_pin_metadata(
                                  l_Type->friendlyName, l_Type->identifier),
                              p_Schema);

              Editor::Pin l_ValueIn = make_input_pin(p_Graph, p_NodeId);
              Pin l_ValueMetadata = make_pin_metadata_from_rtti(
                  l_Property.friendlyName, l_Property.propInfoBase.type,
                  l_Property.propInfoBase.handleType);
              p_Graph.add_pin(l_ValueIn, l_ValueMetadata, p_Schema);
            }

            virtual void compile(Graph &p_Graph, NodeId p_NodeId,
                                 CompileContext &p_CompileContext) const
                override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              PropertyMetadataBase l_Property =
                  get_selected_property(p_Graph, p_NodeId);
              const Pin *l_HandlePin = p_Graph.find_input_pin_checked(
                  p_NodeId, l_Type ? l_Type->friendlyName : "");
              const Pin *l_ValuePin = p_Graph.find_input_pin_checked(
                  p_NodeId, l_Property.friendlyName);
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Then");

              compile_input_pin(p_Graph, p_NodeId, l_HandlePin->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(".");
              p_CompileContext.main_code.append(
                  l_Property.setterName.c_str());
              p_CompileContext.main_code.append("(");
              compile_input_pin(p_Graph, p_NodeId, l_ValuePin->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(");").endl();

              p_Graph.continue_compilation(l_ThenPin->pin,
                                           p_CompileContext);
            }
          };

          struct FunctionNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_handle_function);
            }

            virtual Util::String
            get_title(const Graph &p_Graph, NodeId p_NodeId) const override
            {
              const FunctionMetadata *l_Function =
                  get_selected_function(p_Graph, p_NodeId);
              return l_Function ? l_Function->friendlyName
                                : Util::String("Function");
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph,
                         NodeId p_NodeId) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &, NodeId) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_HandleColor;
            }

            virtual void
            setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                               const NodeGraphSchema *p_Schema) const
                override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              const FunctionMetadata *l_Function =
                  get_selected_function(p_Graph, p_NodeId);
              if (!l_Type || !l_Function) {
                return;
              }

              if (l_Function->hasReturnValue) {
                Editor::Pin l_ResultOut =
                    make_output_pin(p_Graph, p_NodeId);
                Pin l_ResultMetadata = make_pin_metadata_from_rtti(
                    "", l_Function->functionInfo.type,
                    l_Function->functionInfo.handleType);
                l_ResultMetadata.show_default_value_when_unlinked = false;
                p_Graph.add_pin(l_ResultOut, l_ResultMetadata, p_Schema);
              } else {
                Editor::Pin l_ExecIn =
                    make_input_pin(p_Graph, p_NodeId);
                p_Graph.add_pin(l_ExecIn,
                                make_execution_pin_metadata("Exec"),
                                p_Schema);

                Editor::Pin l_ExecOut =
                    make_output_pin(p_Graph, p_NodeId);
                p_Graph.add_pin(l_ExecOut,
                                make_execution_pin_metadata("Then"),
                                p_Schema);
              }

              if (!l_Function->isStatic) {
                Editor::Pin l_InstanceIn =
                    make_input_pin(p_Graph, p_NodeId);
                p_Graph.add_pin(l_InstanceIn,
                                make_handle_pin_metadata(
                                    l_Type->friendlyName,
                                    l_Type->identifier),
                                p_Schema);
              }

              for (const ParameterMetadata &i_Param :
                   l_Function->parameters) {
                Editor::Pin l_ParamIn =
                    make_input_pin(p_Graph, p_NodeId);
                Pin l_ParamMetadata = make_pin_metadata_from_rtti(
                    i_Param.friendlyName, i_Param.paramInfo.type,
                    i_Param.paramInfo.handleType);
                p_Graph.add_pin(l_ParamIn, l_ParamMetadata, p_Schema);
              }
            }

            void append_call(Graph &p_Graph, NodeId p_NodeId,
                             CompileContext &p_CompileContext) const
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              const FunctionMetadata *l_Function =
                  get_selected_function(p_Graph, p_NodeId);
              LOW_ASSERT(l_Type && l_Function,
                         "Missing function metadata for handle function node");
              if (!l_Type || !l_Function) {
                return;
              }

              u32 l_InputOffset = l_Function->hasReturnValue ? 0 : 2;

              if (l_Function->isStatic) {
                p_CompileContext.main_code.append(
                    get_script_type_string(*l_Type).c_str());
                p_CompileContext.main_code.append("::");
              } else {
                const Pin *l_InstancePin = p_Graph.find_input_pin_checked(
                    p_NodeId, l_Type->friendlyName);
                compile_input_pin(p_Graph, p_NodeId, l_InstancePin->pin,
                                  p_CompileContext);
                p_CompileContext.main_code.append(".");
                l_InputOffset += 1;
              }

              p_CompileContext.main_code.append(
                  l_Function->name.c_str());
              p_CompileContext.main_code.append("(");

              for (u32 i = 0; i < l_Function->parameters.size(); ++i) {
                if (i) {
                  p_CompileContext.main_code.append(", ");
                }
                const Pin *l_ParamPin = p_Graph.find_input_pin_checked(
                    p_NodeId, l_Function->parameters[i].friendlyName);
                compile_input_pin(p_Graph, p_NodeId, l_ParamPin->pin,
                                  p_CompileContext);
              }

              p_CompileContext.main_code.append(")");
            }

            virtual void compile(Graph &p_Graph, NodeId p_NodeId,
                                 CompileContext &p_CompileContext) const
                override
            {
              const FunctionMetadata *l_Function =
                  get_selected_function(p_Graph, p_NodeId);
              LOW_ASSERT(l_Function && !l_Function->hasReturnValue,
                         "Tried to statement-compile returning handle "
                         "function node");
              if (!l_Function || l_Function->hasReturnValue) {
                return;
              }

              append_call(p_Graph, p_NodeId, p_CompileContext);
              p_CompileContext.main_code.append(";").endl();

              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Then");
              p_Graph.continue_compilation(l_ThenPin->pin,
                                           p_CompileContext);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              const FunctionMetadata *l_Function =
                  get_selected_function(p_Graph, p_NodeId);
              LOW_ASSERT(l_Function && l_Function->hasReturnValue,
                         "Tried to expression-compile void handle "
                         "function node");
              if (!l_Function || !l_Function->hasReturnValue) {
                return;
              }

              append_call(p_Graph, p_NodeId, p_CompileContext);
            }
          };

          struct ForEachInstanceNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_handle_for_each_instance);
            }

            virtual Util::String
            get_title(const Graph &, NodeId) const override
            {
              return "For each instance";
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph,
                         NodeId p_NodeId) const override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              return l_Type ? l_Type->friendlyName : Util::String("");
            }

            virtual Util::String
            get_category(const Graph &, NodeId) const override
            {
              return "Handle";
            }

            virtual ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_HandleColor;
            }

            virtual void
            setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                               const NodeGraphSchema *p_Schema) const
                override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              if (!l_Type) {
                return;
              }

              Editor::Pin l_ExecIn = make_input_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(l_ExecIn,
                              make_execution_pin_metadata("Exec"),
                              p_Schema);

              Editor::Pin l_ThenOut =
                  make_output_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(l_ThenOut,
                              make_execution_pin_metadata("Then"),
                              p_Schema);

              Editor::Pin l_LoopOut =
                  make_output_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(l_LoopOut,
                              make_execution_pin_metadata("Loop"),
                              p_Schema);

              Editor::Pin l_InstanceOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_InstanceMetadata = make_handle_pin_metadata(
                  "Instance", l_Type->identifier);
              l_InstanceMetadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(l_InstanceOut, l_InstanceMetadata, p_Schema);
            }

            virtual void compile(Graph &p_Graph, NodeId p_NodeId,
                                 CompileContext &p_CompileContext) const
                override
            {
              const TypeMetadata *l_Type =
                  get_selected_type(p_Graph, p_NodeId);
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Then");
              const Pin *l_LoopPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Loop");
              LOW_ASSERT(l_Type,
                         "Missing selected type for foreach node");
              if (!l_Type) {
                return;
              }

              Util::StringBuilder l_IndexVariableName;
              l_IndexVariableName.append("__foreach_index")
                  .append((u64)p_NodeId.value);
              Util::StringBuilder l_InstanceVariableName;
              l_InstanceVariableName.append("__foreach_instance")
                  .append((u64)p_NodeId.value);

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
              p_Graph.continue_compilation(l_LoopPin->pin,
                                           p_CompileContext);
              p_CompileContext.main_code.append("}").endl();

              p_Graph.continue_compilation(l_ThenPin->pin,
                                           p_CompileContext);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_Graph;
              (void)p_PinId;
              Util::StringBuilder l_InstanceVariableName;
              l_InstanceVariableName.append("__foreach_instance")
                  .append((u64)p_NodeId.value);
              p_CompileContext.main_code.append(
                  l_InstanceVariableName.get());
            }
          };

          static TypeIdNodeClass g_TypeIdNodeClass;
          static InstanceCountNodeClass g_InstanceCountNodeClass;
          static GetInstanceByIndexNodeClass g_GetInstanceByIndexNodeClass;
          static FindByNameNodeClass g_FindByNameNodeClass;
          static DestroyNodeClass g_DestroyNodeClass;
          static GetPropertyNodeClass g_GetPropertyNodeClass;
          static SetPropertyNodeClass g_SetPropertyNodeClass;
          static FunctionNodeClass g_FunctionNodeClass;
          static ForEachInstanceNodeClass g_ForEachInstanceNodeClass;

          static void register_spawn_entry(Graph &p_Graph, Util::String p_Category,
                                           Util::String p_Title,
                                           Util::String p_Subtitle,
                                           Util::String p_SearchText,
                                           Util::Name p_NodeClass,
                                           Util::TypeIdentifier p_HandleType,
                                           Util::Name p_MemberName = {})
          {
            NodeSpawnEntry l_Entry;
            Util::String l_IdString = "vs_spawn_handle_";
            l_IdString += p_NodeClass.c_str();
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
            l_Entry.node_class = p_NodeClass;
            l_Entry.initialize_node =
                [p_HandleType, p_MemberName](Graph &, Node &p_Node) {
                  p_Node.handle_type = p_HandleType;
                  p_Node.member_name = p_MemberName;
                };
            p_Graph.register_spawn_entry(l_Entry);
          }
        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          p_Graph.register_node_class(g_TypeIdNodeClass);
          p_Graph.register_node_class(g_InstanceCountNodeClass);
          p_Graph.register_node_class(g_GetInstanceByIndexNodeClass);
          p_Graph.register_node_class(g_FindByNameNodeClass);
          p_Graph.register_node_class(g_DestroyNodeClass);
          p_Graph.register_node_class(g_GetPropertyNodeClass);
          p_Graph.register_node_class(g_SetPropertyNodeClass);
          p_Graph.register_node_class(g_FunctionNodeClass);
          p_Graph.register_node_class(g_ForEachInstanceNodeClass);

          for (auto &it : get_type_metadata()) {
            const TypeMetadata &i_Type = it.second;
            if (!i_Type.scriptingExpose) {
              continue;
            }

            register_spawn_entry(
                p_Graph, i_Type.friendlyName, "Type ID",
                i_Type.friendlyName,
                Util::String("type id ") + i_Type.friendlyName,
                g_TypeIdNodeClass.get_name(), i_Type.identifier);
            register_spawn_entry(
                p_Graph, i_Type.friendlyName, "Instance count",
                i_Type.friendlyName,
                Util::String("instance count ") + i_Type.friendlyName,
                g_InstanceCountNodeClass.get_name(), i_Type.identifier);
            register_spawn_entry(
                p_Graph, i_Type.friendlyName,
                "Get living instance by index", i_Type.friendlyName,
                Util::String("get living instance index ") +
                    i_Type.friendlyName,
                g_GetInstanceByIndexNodeClass.get_name(),
                i_Type.identifier);
            register_spawn_entry(
                p_Graph, i_Type.friendlyName, "For each instance",
                i_Type.friendlyName,
                Util::String("for each instance ") + i_Type.friendlyName,
                g_ForEachInstanceNodeClass.get_name(),
                i_Type.identifier);
            register_spawn_entry(
                p_Graph, i_Type.friendlyName, "Find by name",
                i_Type.friendlyName,
                Util::String("find by name ") + i_Type.friendlyName,
                g_FindByNameNodeClass.get_name(), i_Type.identifier);
            register_spawn_entry(
                p_Graph, i_Type.friendlyName, "Destroy",
                i_Type.friendlyName,
                Util::String("destroy ") + i_Type.friendlyName,
                g_DestroyNodeClass.get_name(), i_Type.identifier);

            for (const PropertyMetadata &i_Property : i_Type.properties) {
              if (!i_Property.scriptingExpose || i_Property.hideFlode) {
                continue;
              }

              if (!i_Property.hideGetterFlode) {
                register_spawn_entry(
                    p_Graph, i_Type.friendlyName,
                    Util::String("Get ") + i_Property.friendlyName,
                    i_Type.friendlyName,
                    Util::String("get ") + i_Property.friendlyName +
                        " " + i_Type.friendlyName,
                    g_GetPropertyNodeClass.get_name(),
                    i_Type.identifier, i_Property.name);
              }

              if (!i_Property.hideSetterFlode) {
                register_spawn_entry(
                    p_Graph, i_Type.friendlyName,
                    Util::String("Set ") + i_Property.friendlyName,
                    i_Type.friendlyName,
                    Util::String("set ") + i_Property.friendlyName +
                        " " + i_Type.friendlyName,
                    g_SetPropertyNodeClass.get_name(),
                    i_Type.identifier, i_Property.name);
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
                  Util::String("get ") + i_Property.friendlyName + " " +
                      i_Type.friendlyName,
                  g_GetPropertyNodeClass.get_name(),
                  i_Type.identifier, i_Property.name);
              register_spawn_entry(
                  p_Graph, i_Type.friendlyName,
                  Util::String("Set ") + i_Property.friendlyName,
                  i_Type.friendlyName,
                  Util::String("set ") + i_Property.friendlyName + " " +
                      i_Type.friendlyName,
                  g_SetPropertyNodeClass.get_name(),
                  i_Type.identifier, i_Property.name);
            }

            for (const FunctionMetadata &i_Function : i_Type.functions) {
              if (!i_Function.scriptingExpose || i_Function.hideFlode) {
                continue;
              }

              register_spawn_entry(
                  p_Graph, i_Type.friendlyName,
                  i_Function.friendlyName, i_Type.friendlyName,
                  i_Function.friendlyName + " " + i_Type.friendlyName,
                  g_FunctionNodeClass.get_name(), i_Type.identifier,
                  i_Function.name);
            }
          }
        }
      } // namespace HandleNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
