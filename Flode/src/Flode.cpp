#include "Flode.h"

#include "FlodeHandleNodes.h"
#include "FlodeHelpers.h"

#include "LowUtil.h"
#include "LowUtilLogger.h"
#include "LowUtilSerialization.h"
#include "LowUtilFileIO.h"

#include "LowEditor.h"
#include "LowEditorPropertyEditors.h"
#include "LowEditorBase.h"
#include "LowEditorMainWindow.h"

#include "LowRendererImGuiHelper.h"

#include "utilities/drawing.h"
#include "utilities/widgets.h"

namespace Flode {

  ImVec4 g_NodePadding(4, 2, 4, 4);
  int m_PinIconSize = 20;

  Low::Util::List<Low::Editor::TypeMetadata> g_ExposedTypes;

  Low::Util::Map<Low::Util::String,
                 Low::Util::Map<Low::Util::String, Low::Util::Name>>
      g_NodeTypes;

  Low::Util::Map<PinType, Low::Util::Map<PinType, Low::Util::Name>>
      g_CastNodes;

  Low::Util::Map<Low::Util::Name, std::function<Node *()>>
      g_NodeTypeNames;

  void ShowToolTip(const char *p_Label, ImColor p_Color)
  {
    // ImGui::SetTooltip(p_Label);
    // return;
    ImGui::SetCursorPosY(ImGui::GetMousePos().y -
                         ImGui::GetTextLineHeight() * 2);
    auto l_Size = ImGui::CalcTextSize(p_Label);

    auto l_Padding = ImGui::GetStyle().FramePadding;
    auto l_Spacing = ImGui::GetStyle().ItemSpacing;

    ImGui::SetCursorPos(ImGui::GetMousePos() +
                        ImVec2(l_Spacing.x, -l_Spacing.y));

    auto l_RectMin = ImGui::GetMousePos() - l_Padding;
    auto l_RectMax = ImGui::GetMousePos() + l_Size + l_Padding;

    auto l_DrawList = ImGui::GetWindowDrawList();
    l_DrawList->AddRectFilled(l_RectMin, l_RectMax, p_Color,
                              l_Size.y * 0.15f);
    ImGui::TextUnformatted(p_Label);
  }

  void initialize()
  {
    for (auto it = Low::Editor::get_type_metadata().begin();
         it != Low::Editor::get_type_metadata().end(); ++it) {

      if (it->second.scriptingExpose) {
        g_ExposedTypes.push_back(it->second);
      }
    }
  }

  Low::Util::List<Low::Editor::TypeMetadata> &get_exposed_types()
  {
    return g_ExposedTypes;
  }

  Low::Util::String get_pin_default_value_as_string(Pin *p_Pin)
  {
    switch (p_Pin->type) {
    case PinType::Handle:
      return "0ull";
    case PinType::Enum: {
      Low::Util::RTTI::EnumInfo l_EnumInfo =
          Low::Util::get_enum_info(p_Pin->typeId);
      Low::Editor::EnumMetadata &l_EnumMetadata =
          Low::Editor::get_enum_metadata(p_Pin->typeId);
      Low::Util::String l_EnumEntryString =
          l_EnumInfo.entry_name(p_Pin->defaultValue.m_Uint32).c_str();
      Low::Util::String l_EnumDefaultValue =
          l_EnumMetadata.fullTypeString;
      l_EnumEntryString.make_upper();
      l_EnumDefaultValue += "::";
      l_EnumDefaultValue += l_EnumEntryString;

      return l_EnumDefaultValue;
    }
    case PinType::Number:
      return (LOW_TO_STRING(p_Pin->defaultValue.m_Float) + "f");
    case PinType::Bool:
      return p_Pin->defaultValue.m_Bool ? "true" : "false";
    case PinType::String: {
      if (p_Pin->stringType == PinStringType::Name) {
        return Low::Util::String("_LNAME(") +
               Low::Util::Name(p_Pin->defaultValue.m_Uint32).c_str() +
               ")";
      }
      return Low::Util::String("Low::Util::String(\"") +
             p_Pin->defaultStringValue + "\")";
    }
    case PinType::Vector2: {
      Low::Util::StringBuilder l_Builder;
      Low::Math::Vector2 l_Vec = p_Pin->defaultValue.m_Vector2;
      l_Builder.append("Low::Math::Vector2(");
      l_Builder.append(l_Vec.x).append("f, ");
      l_Builder.append(l_Vec.y).append("f)");
      return l_Builder.get();
    }
    case PinType::Vector3: {
      Low::Util::StringBuilder l_Builder;
      Low::Math::Vector3 l_Vec = p_Pin->defaultValue.m_Vector3;
      l_Builder.append("Low::Math::Vector3(");
      l_Builder.append(l_Vec.x).append("f, ");
      l_Builder.append(l_Vec.y).append("f, ");
      l_Builder.append(l_Vec.z).append("f)");
      return l_Builder.get();
    }
    case PinType::Quaternion: {
      Low::Util::StringBuilder l_Builder;
      Low::Math::Quaternion l_Quat = p_Pin->defaultValue.m_Quaternion;
      l_Builder.append("Low::Math::Quaternion(");
      l_Builder.append(l_Quat.x).append("f, ");
      l_Builder.append(l_Quat.y).append("f, ");
      l_Builder.append(l_Quat.z).append("f, ");
      l_Builder.append(l_Quat.w).append("f)");
      return l_Builder.get();
    }
    default:
      break;
    }

    return "";
  }

  void setup_default_value_for_pin(Pin *p_Pin)
  {
    switch (p_Pin->type) {
    case PinType::Number: {
      p_Pin->defaultValue.m_Type = Low::Util::VariantType::Float;
      p_Pin->defaultValue.m_Float = 0.0f;
      break;
    }
    case PinType::Bool: {
      p_Pin->defaultValue.m_Type = Low::Util::VariantType::Bool;
      p_Pin->defaultValue.m_Bool = false;
      break;
    }
    case PinType::Handle: {
      p_Pin->defaultValue.m_Type = Low::Util::VariantType::Handle;
      p_Pin->defaultValue.m_Uint64 = 0ull;
      break;
    }
    case PinType::Enum: {
      p_Pin->defaultValue.m_Type = Low::Util::VariantType::UInt32;
      p_Pin->defaultValue.m_Int32 = 0ull;
      break;
    }
    case PinType::String: {
      if (p_Pin->stringType == PinStringType::Name) {
        p_Pin->defaultValue.m_Type = Low::Util::VariantType::Name;
        p_Pin->defaultValue = LOW_NAME("");
      } else {
        p_Pin->defaultValue.m_Type = Low::Util::VariantType::String;
        p_Pin->defaultValue.m_Int32 = 0;
        p_Pin->defaultStringValue = "";
      }
      break;
    }
    case PinType::Vector2: {
      p_Pin->defaultValue.m_Type = Low::Util::VariantType::Vector2;
      p_Pin->defaultValue.m_Vector2 = Low::Math::Vector2(0.0f, 0.0f);
      break;
    }
    case PinType::Vector3: {
      p_Pin->defaultValue.m_Type = Low::Util::VariantType::Vector3;
      p_Pin->defaultValue.m_Vector3 =
          Low::Math::Vector3(0.0f, 0.0f, 0.0f);
      break;
    }
    case PinType::Quaternion: {
      p_Pin->defaultValue.m_Type = Low::Util::VariantType::Quaternion;
      p_Pin->defaultValue.m_Quaternion =
          Low::Math::Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
      break;
    }
    default: {
      p_Pin->defaultValue.m_Type = Low::Util::VariantType::String;
      p_Pin->defaultValue.m_Int32 = 0;
      break;
    }
    }
  }

  Low::Util::String pin_direction_to_string(PinDirection p_Direction)
  {
    switch (p_Direction) {
    case PinDirection::Input:
      return "input";
    case PinDirection::Output:
      return "output";
    }

    _LOW_ASSERT(false);
    return "";
  }

  Low::Util::String pin_type_to_string(PinType p_Type)
  {
    switch (p_Type) {
    case PinType::Flow:
      return "flow";
    case PinType::Number:
      return "number";
    case PinType::Bool:
      return "bool";
    case PinType::Handle:
      return "handle";
    case PinType::Enum:
      return "enum";
    case PinType::String:
      return "string";
    case PinType::Vector2:
      return "vector2";
    case PinType::Vector3:
      return "vector3";
    case PinType::Quaternion:
      return "quaternion";
    case PinType::Dynamic:
      return "dynamic";
    }

    _LOW_ASSERT(false);
    return "";
  }

  PinType string_to_pin_type(Low::Util::String p_String)
  {
    if (p_String == "flow") {
      return PinType::Flow;
    }
    if (p_String == "number") {
      return PinType::Number;
    }
    if (p_String == "bool") {
      return PinType::Bool;
    }
    if (p_String == "string") {
      return PinType::String;
    }
    if (p_String == "handle") {
      return PinType::Handle;
    }
    if (p_String == "enum") {
      return PinType::Enum;
    }
    if (p_String == "vector2") {
      return PinType::Vector2;
    }
    if (p_String == "vector3") {
      return PinType::Vector3;
    }
    if (p_String == "quaternion") {
      return PinType::Quaternion;
    }
    if (p_String == "dynamic") {
      return PinType::Dynamic;
    }

    _LOW_ASSERT(false);
  }

  PinType property_type_to_pin_type(u8 p_PropertyType)
  {
    using namespace Low::Util::RTTI;

    switch (p_PropertyType) {
    case PropertyType::HANDLE:
      return PinType::Handle;
    case PropertyType::ENUM:
      return PinType::Enum;
    case PropertyType::BOOL:
      return PinType::Bool;
    case PropertyType::UINT16:
      return PinType::Number;
    case PropertyType::INT:
      return PinType::Number;
    case PropertyType::UINT32:
      return PinType::Number;
    case PropertyType::FLOAT:
      return PinType::Number;
    case PropertyType::STRING:
      return PinType::String;
    case PropertyType::NAME:
      return PinType::String;
    case PropertyType::VECTOR2:
      return PinType::Vector2;
    case PropertyType::VECTOR3:
      return PinType::Vector3;
    default: {
      LOW_ASSERT(false, "Unsupported property type");
      return PinType::String;
    }
    }
  }

  PinStringType property_type_to_pin_string_type(u8 p_PropertyType)
  {
    using namespace Low::Util::RTTI;

    switch (p_PropertyType) {
    case PropertyType::STRING:
      return PinStringType::String;
    case PropertyType::NAME:
      return PinStringType::Name;
    default: {
      LOW_ASSERT(false, "Unsupported property type for string");
      return PinStringType::String;
    }
    }
  }

  PinType variant_type_to_pin_type(u8 p_VariantType)
  {
    switch (p_VariantType) {
    case Low::Util::VariantType::Bool:
      return PinType::Bool;
    case Low::Util::VariantType::Int32:
      return PinType::Number;
    case Low::Util::VariantType::UInt32:
      return PinType::Number;
    case Low::Util::VariantType::Vector2:
      return PinType::Vector2;
    case Low::Util::VariantType::Vector3:
      return PinType::Vector3;
    case Low::Util::VariantType::Float:
      return PinType::Number;
    default: {
      LOW_ASSERT(false,
                 "Unsupported variant type to pin type conversion");
      return PinType::Bool;
    }
    }
  }

  void register_nodes_for_type(u16 p_TypeId)
  {
    Low::Util::String l_NodeNamePrefix = "Handle_";
    l_NodeNamePrefix += LOW_TO_STRING(p_TypeId);
    l_NodeNamePrefix += "_";

    Low::Util::RTTI::TypeInfo l_TypeInfo =
        Low::Util::Handle::get_type_info(p_TypeId);
    Low::Editor::TypeMetadata l_TypeMetadata =
        Low::Editor::get_type_metadata(p_TypeId);

    {
      Low::Util::Name l_NodeTypeName = LOW_NAME(
          Low::Util::String(l_NodeNamePrefix + "TypeId").c_str());
      Flode::register_node(
          l_NodeTypeName, [p_TypeId]() -> Flode::Node * {
            return new Flode::HandleNodes::TypeIdNode(p_TypeId);
          });

      Flode::register_spawn_node(l_TypeInfo.name.c_str(), "Type ID",
                                 l_NodeTypeName);
    }

    {
      Low::Util::Name l_NodeTypeName = LOW_NAME(
          Low::Util::String(l_NodeNamePrefix + "FindByName").c_str());
      Flode::register_node(
          l_NodeTypeName, [p_TypeId]() -> Flode::Node * {
            return new Flode::HandleNodes::FindByNameNode(p_TypeId);
          });

      Flode::register_spawn_node(l_TypeInfo.name.c_str(),
                                 "Find by name", l_NodeTypeName);
    }

    {
      for (auto it = l_TypeMetadata.properties.begin();
           it != l_TypeMetadata.properties.end(); ++it) {
        if (!it->scriptingExpose) {
          continue;
        }

        Low::Util::Name i_PropertyName = it->name;

        {
          Low::Util::Name i_NodeTypeName =
              LOW_NAME(Low::Util::String(l_NodeNamePrefix + "Get" +
                                         i_PropertyName.c_str())
                           .c_str());
          Flode::register_node(
              i_NodeTypeName,
              [p_TypeId, i_PropertyName]() -> Flode::Node * {
                return new Flode::HandleNodes::GetNode(
                    p_TypeId, i_PropertyName);
              });

          Flode::register_spawn_node(l_TypeInfo.name.c_str(),
                                     Low::Util::String("Get ") +
                                         i_PropertyName.c_str(),
                                     i_NodeTypeName);
        }
        {
          Low::Util::Name i_NodeTypeName =
              LOW_NAME(Low::Util::String(l_NodeNamePrefix + "Set" +
                                         i_PropertyName.c_str())
                           .c_str());
          Flode::register_node(
              i_NodeTypeName,
              [p_TypeId, i_PropertyName]() -> Flode::Node * {
                return new Flode::HandleNodes::SetNode(
                    p_TypeId, i_PropertyName);
              });

          Flode::register_spawn_node(l_TypeInfo.name.c_str(),
                                     Low::Util::String("Set ") +
                                         i_PropertyName.c_str(),
                                     i_NodeTypeName);
        }
      }
    }
    {
      for (auto it = l_TypeMetadata.functions.begin();
           it != l_TypeMetadata.functions.end(); ++it) {
        if (!it->scriptingExpose || it->hideFlode) {
          continue;
        }
        Low::Util::Name i_FunctionName = it->name;
        Low::Util::Name i_NodeTypeName =
            LOW_NAME(Low::Util::String(l_NodeNamePrefix + "Function" +
                                       it->name.c_str())
                         .c_str());
        Flode::register_node(
            i_NodeTypeName,
            [p_TypeId, i_FunctionName]() -> Flode::Node * {
              return new Flode::HandleNodes::FunctionNode(
                  p_TypeId, i_FunctionName);
            });

        Flode::register_spawn_node(l_TypeInfo.name.c_str(),
                                   it->friendlyName, i_NodeTypeName);
      }
    }
  }

  void register_node(Low::Util::Name p_TypeName,
                     std::function<Node *()> p_Callback)
  {
    LOW_ASSERT(g_NodeTypeNames.find(p_TypeName) ==
                   g_NodeTypeNames.end(),
               "A node type with this typename has already been "
               "registered.");

    g_NodeTypeNames[p_TypeName] = p_Callback;
  }

  void register_spawn_node(Low::Util::String p_Category,
                           Low::Util::String p_Name,
                           Low::Util::Name p_TypeName)
  {
    LOW_ASSERT(g_NodeTypeNames.find(p_TypeName) !=
                   g_NodeTypeNames.end(),
               "Node was not registered before added as a "
               "spawnable node.");
    g_NodeTypes[p_Category][p_Name] = p_TypeName;
  }

  void register_cast_node(PinType p_FromType, PinType p_ToType,
                          Low::Util::Name p_TypeName)
  {
    LOW_ASSERT(
        g_NodeTypeNames.find(p_TypeName) != g_NodeTypeNames.end(),
        "Node was not registered before added as a cast node.");
    g_CastNodes[p_FromType][p_ToType] = p_TypeName;
  }

  bool can_cast(PinType p_FromType, PinType p_ToType)
  {
    auto l_FromEntry = g_CastNodes.find(p_FromType);
    if (l_FromEntry == g_CastNodes.end()) {
      return false;
    }

    auto l_ToEntry = l_FromEntry->second.find(p_ToType);
    return l_ToEntry != l_FromEntry->second.end();
  }

  Low::Util::Name get_cast_node_typename(PinType p_FromType,
                                         PinType p_ToType)
  {
    auto l_FromEntry = g_CastNodes.find(p_FromType);
    LOW_ASSERT(l_FromEntry != g_CastNodes.end(),
               "No cast node registrered");

    auto l_ToEntry = l_FromEntry->second.find(p_ToType);
    LOW_ASSERT(l_ToEntry != l_FromEntry->second.end(),
               "No cast node registered");

    return l_ToEntry->second;
  }

  Low::Util::Map<Low::Util::String,
                 Low::Util::Map<Low::Util::String, Low::Util::Name>> &
  get_node_types()
  {
    return g_NodeTypes;
  }

  Node *spawn_node_of_type(Low::Util::Name p_TypeName)
  {
    Node *l_Node = g_NodeTypeNames[p_TypeName]();
    l_Node->typeName = p_TypeName;

    return l_Node;
  }

  static ImColor get_icon_color(Flode::PinType type)
  {
    using namespace Flode;

    switch (type) {
    default:
    case PinType::Flow:
      return ImColor(255, 255, 255);
    case PinType::Bool:
      return ImColor(220, 48, 48);
    case PinType::Vector2:
      return ImColor(68, 150, 126);
    case PinType::Vector3:
      return ImColor(68, 201, 156);
    case PinType::Quaternion:
      return ImColor(201, 201, 156);
    case PinType::Number:
      return ImColor(147, 226, 74);
    case PinType::String:
      return ImColor(124, 21, 153);
    case PinType::Handle:
      return ImColor(51, 150, 215);
    case PinType::Enum:
      return ImColor(218, 0, 183);
    case PinType::Dynamic:
      return ImColor(120, 120, 120);
      /*
    case PinType::Function:
      return ImColor(218, 0, 183);
    case PinType::Delegate:
      return ImColor(255, 48, 48);
      */
    }
  };

  static void draw_pin_icon(const Flode::Pin *pin, bool connected,
                            int alpha)
  {
    using namespace ax::Drawing;

    IconType iconType;
    ImColor color = get_icon_color(pin->type);
    color.Value.w = alpha / 255.0f;
    switch (pin->type) {
    case Flode::PinType::Flow:
      iconType = IconType::Flow;
      break;
    case Flode::PinType::Bool:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::Vector2:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::Vector3:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::Quaternion:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::Dynamic:
      iconType = IconType::Circle;
      break;
      /*
    case Flode::PinType::Int:
      iconType = IconType::Circle;
      break;
      */
    case Flode::PinType::Number:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::String:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::Handle:
      iconType = IconType::Circle;
    case Flode::PinType::Enum:
      iconType = IconType::Circle;
      break;
      /*
    case Flode::PinType::Function:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::Delegate:
      iconType = IconType::Square;
      break;
      */
    default:
      return;
    }

    ax::Widgets::Icon(ImVec2(static_cast<float>(m_PinIconSize),
                             static_cast<float>(m_PinIconSize)),
                      iconType, connected, color,
                      ImColor(32, 32, 32, alpha));
  };

  Node::~Node()
  {
    for (auto it = pins.begin(); it != pins.end();) {
      Pin *i_Pin = *it;

      it = pins.erase(it);
      delete i_Pin;
    }
  }

  Low::Util::String Node::get_name(NodeNameType p_Type) const
  {
    return "FlodeNode";
  }

  inline void compile_string_pin_connection_conversion(
      Low::Util::StringBuilder &p_Builder, const Node *p_OutputNode,
      Pin *p_OutputPin, const Node *p_InputNode, Pin *p_InputPin)
  {
    // No conversion needed
    if (p_OutputPin->stringType == p_InputPin->stringType) {
      p_OutputNode->compile_output_pin(p_Builder, p_OutputPin->id);
      return;
    }

    // Convert from name to string
    if (p_OutputPin->stringType == PinStringType::Name &&
        p_InputPin->stringType == PinStringType::String) {
      p_Builder.append("Low::Util::String(");
      p_OutputNode->compile_output_pin(p_Builder, p_OutputPin->id);
      p_Builder.append(".c_str())");
      return;
    }

    // Convert from string to name
    if (p_OutputPin->stringType == PinStringType::String &&
        p_InputPin->stringType == PinStringType::Name) {
      p_Builder.append("Low::Util::Name::from_string(");
      p_OutputNode->compile_output_pin(p_Builder, p_OutputPin->id);
      p_Builder.append(")");
      return;
    }

    _LOW_ASSERT(false);
  }

  void Node::compile_input_pin(Low::Util::StringBuilder &p_Builder,
                               NodeEd::PinId p_PinId) const
  {
    Pin *l_Pin = find_pin_checked(p_PinId);

    LOW_ASSERT(l_Pin->direction == PinDirection::Input,
               "Pin is not an input pin");

    if (graph->is_pin_connected(l_Pin->id)) {

      Pin *l_ConnectedPin =
          graph->find_pin(graph->get_connected_pin(l_Pin->id));
      Node *l_ConnectedNode =
          graph->find_node(l_ConnectedPin->nodeId);

      if (l_Pin->type == PinType::String) {
        compile_string_pin_connection_conversion(
            p_Builder, l_ConnectedNode, l_ConnectedPin, this, l_Pin);
      } else {
        l_ConnectedNode->compile_output_pin(p_Builder,
                                            l_ConnectedPin->id);
      }

    } else {
      p_Builder.append(get_pin_default_value_as_string(l_Pin));
    }
  }

  Pin *Node::find_pin_checked(NodeEd::PinId p_PinId) const
  {
    Pin *l_Pin = find_pin(p_PinId);

    LOW_ASSERT(l_Pin, "Could not find pin by id");

    return l_Pin;
  }

  Pin *Node::find_output_pin_checked(NodeEd::PinId p_PinId) const
  {
    Pin *l_Pin = find_pin_checked(p_PinId);

    LOW_ASSERT(l_Pin->direction == PinDirection::Output,
               "Expected pin to be an output pin.");

    return l_Pin;
  }

  Pin *Node::find_pin(NodeEd::PinId p_PinId) const
  {
    for (Pin *i_Pin : pins) {
      if (i_Pin->id == p_PinId) {
        return i_Pin;
      }
    }

    return nullptr;
  }

  Pin *Node::create_pin(PinDirection p_Direction,
                        Low::Util::String p_Title, PinType p_Type,
                        u16 p_TypeId, u64 p_PinId)
  {
    Pin *l_Pin = new Pin;

    l_Pin->title = p_Title;
    if (p_PinId == 0) {

      l_Pin->id = graph->m_IdCounter++;
    } else {
      l_Pin->id = p_PinId;
    }

    l_Pin->type = p_Type;
    l_Pin->typeId = p_TypeId;
    l_Pin->direction = p_Direction;
    l_Pin->nodeId = id;
    setup_default_value_for_pin(l_Pin);

    pins.push_back(l_Pin);

    return l_Pin;
  }

  Pin *Node::create_pin(PinDirection p_Direction,
                        Low::Util::String p_Title, PinType p_Type,
                        u64 p_PinId)
  {
    return create_pin(p_Direction, p_Title, p_Type, 0, p_PinId);
  }

  Pin *Node::create_string_pin(PinDirection p_Direction,
                               Low::Util::String p_Title,
                               PinStringType p_StringType,
                               u64 p_PinId)
  {
    Pin *l_Pin =
        create_pin(p_Direction, p_Title, PinType::String, 0, p_PinId);
    l_Pin->stringType = p_StringType;
    setup_default_value_for_pin(l_Pin);

    return l_Pin;
  }

  Pin *Node::create_handle_pin(PinDirection p_Direction,
                               Low::Util::String p_Title,
                               u16 p_TypeId, u64 p_PinId)
  {
    return create_pin(p_Direction, p_Title, PinType::Handle, p_TypeId,
                      p_PinId);
  }

  Pin *Node::create_enum_pin(PinDirection p_Direction,
                             Low::Util::String p_Title, u16 p_EnumId,
                             u64 p_PinId)
  {
    return create_pin(p_Direction, p_Title, PinType::Enum, p_EnumId,
                      p_PinId);
  }

  Pin *Node::create_pin_from_rtti(PinDirection p_Direction,
                                  Low::Util::String p_Title,
                                  u32 p_PropertyType, u16 p_TypeId,
                                  u64 p_PinId)
  {
    if (p_PropertyType == Low::Util::RTTI::PropertyType::HANDLE) {
      return create_handle_pin(p_Direction, p_Title, p_TypeId,
                               p_PinId);
    }

    if (p_PropertyType == Low::Util::RTTI::PropertyType::ENUM) {
      return create_enum_pin(p_Direction, p_Title, p_TypeId, p_PinId);
    }

    PinType l_PinType = property_type_to_pin_type(p_PropertyType);

    if (l_PinType == PinType::String) {
      return create_string_pin(
          p_Direction, p_Title,
          property_type_to_pin_string_type(p_PropertyType), p_PinId);
    }

    return create_pin(p_Direction, p_Title,
                      property_type_to_pin_type(p_PropertyType),
                      p_PinId);
  }

  Pin *Node::create_pin_from_property_info(
      PinDirection p_Direction, Low::Util::String p_Title,
      Low::Util::RTTI::PropertyInfo &p_PropertyInfo, u64 p_PinId)
  {
    return create_pin_from_rtti(p_Direction, p_Title,
                                p_PropertyInfo.type,
                                p_PropertyInfo.handleType, p_PinId);
  }

  Pin *Node::create_pin_from_variant(PinDirection p_Direction,
                                     Low::Util::String p_Title,
                                     Low::Util::Variant &p_Variant,
                                     u64 p_PinId)
  {
    Pin *l_Pin = nullptr;
    if (p_Variant.m_Type == Low::Util::VariantType::Handle) {
      Low::Util::Handle l_Handle = p_Variant.m_Uint64;
      l_Pin = create_handle_pin(p_Direction, p_Title,
                                l_Handle.get_type(), p_PinId);
    } else if (p_Variant.m_Type == Low::Util::VariantType::Name) {
      l_Pin = create_string_pin(p_Direction, p_Title,
                                PinStringType::Name, p_PinId);
    } else {
      l_Pin = create_pin(p_Direction, p_Title,
                         variant_type_to_pin_type(p_Variant.m_Type),
                         p_PinId);
    }

    if (l_Pin) {
      l_Pin->defaultValue = p_Variant;
    }

    return l_Pin;
  }

  void Node::render_header()
  {
    ImGui::BeginHorizontal("header");

    ImGui::Spring(0);
    ImGui::TextUnformatted(get_name(NodeNameType::Full).c_str());
    ImGui::Spring(1);
    ImGui::Dummy(ImVec2(0, 20));
    ImGui::Spring(0);

    ImGui::EndHorizontal();

    m_HeaderMin = ImGui::GetItemRectMin();
    m_HeaderMax = ImGui::GetItemRectMax();
  }

  void Node::render_header_cosmetics()
  {
    ImVec2 l_Max = ImGui::GetItemRectMax();

    const auto l_HalfBorderWidth =
        NodeEd::GetStyle().NodeBorderWidth * 0.5f;

    auto l_DrawList = NodeEd::GetNodeBackgroundDrawList(id);

    l_DrawList->AddRectFilled(
        m_HeaderMin - ImVec2(g_NodePadding.z - l_HalfBorderWidth,
                             g_NodePadding.y - l_HalfBorderWidth),
        ImVec2(l_Max.x - 4.0f, m_HeaderMax.y) +
            ImVec2(g_NodePadding.x - l_HalfBorderWidth, 0),
        get_color(), NodeEd::GetStyle().NodeRounding,
        ImDrawFlags_RoundCornersTop);
  }

  void Node::render_pin(Flode::Pin *p_Pin)
  {
    bool l_Connected = graph->is_pin_connected(p_Pin->id);

    if (p_Pin->direction == Flode::PinDirection::Input) {
      NodeEd::BeginPin(p_Pin->id, NodeEd::PinKind::Input);
    } else {
      ImGui::Spring(0);
      NodeEd::BeginPin(p_Pin->id, NodeEd::PinKind::Output);
    }

    auto alpha = ImGui::GetStyle().Alpha;

    ImGui::BeginHorizontal(p_Pin->id.AsPointer());
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

    if (p_Pin->direction == Flode::PinDirection::Input) {
      draw_pin_icon(p_Pin, l_Connected, (int)(alpha * 255));
      ImGui::Spring(0);
      if (!p_Pin->title.empty()) {
        ImGui::TextUnformatted(p_Pin->title.c_str());
        ImGui::Spring(0);
      }
      if (!l_Connected) {
        if (p_Pin->type == PinType::Number) {
          ImGui::PushItemWidth(50.0f);
          Low::Editor::Base::VariantEdit("##editdefaultvalue",
                                         p_Pin->defaultValue);
          ImGui::PopItemWidth();
        } else if (p_Pin->type == PinType::Bool) {
          Low::Editor::Base::VariantEdit("##editdefaultvalue",
                                         p_Pin->defaultValue);
        } else if (p_Pin->type == PinType::Vector3) {
          Low::Editor::Base::Vector3Edit(
              "##editdefaultvalue", &p_Pin->defaultValue.m_Vector3,
              200.0f);
        } else if (p_Pin->type == PinType::String) {
          ImGui::PushItemWidth(50.0f);
          if (p_Pin->stringType == PinStringType::Name) {
            Low::Editor::Base::NameEdit(
                "##editdefaultvalue",
                (Low::Util::Name *)&p_Pin->defaultValue.m_Uint32);
          } else {
            Low::Editor::Base::StringEdit("##editdefaultvalue",
                                          &p_Pin->defaultStringValue);
          }
          ImGui::PopItemWidth();
        } else if (p_Pin->type == PinType::Enum) {
          // Unfortunately it is right now not easy to align the
          // popup from the dropdown with the node. I'll leave that
          // for the future
          if (p_Pin->typeId != 0) {
            ImGui::PushItemWidth(100.0f);

            // Low::Editor::PropertyEditors::render_enum_selector(p_Pin->typeId,
            //  (u8 *)&p_Pin->defaultValue.m_Uint32,
            //  "##editdefaultvalue", false);

            Low::Util::RTTI::EnumInfo &l_EnumInfo =
                Low::Util::get_enum_info(p_Pin->typeId);

            u8 l_EnumValue = p_Pin->defaultValue.m_Uint32;

            if (Flode::Helper::BeginNodeCombo(
                    "##editdefaultvalue",
                    l_EnumInfo.entry_name(l_EnumValue).c_str())) {
              for (auto it : l_EnumInfo.entries) {
                if (ImGui::Selectable(it.name.c_str(),
                                      l_EnumValue == it.value)) {
                  l_EnumValue = it.value;
                }
              }

              Flode::Helper::EndNodeCombo();
            }

            p_Pin->defaultValue.m_Uint32 = l_EnumValue;
            ImGui::PopItemWidth();
          }
        }
      }
    } else {
      if (!p_Pin->title.empty()) {
        ImGui::Spring(0);
        ImGui::TextUnformatted(p_Pin->title.c_str());
      }
      ImGui::Spring(0);
      draw_pin_icon(p_Pin, l_Connected, (int)(alpha * 255));
    }
    ImGui::PopStyleVar();
    ImGui::EndHorizontal();
    NodeEd::EndPin();
  }

  void Node::render_input_pins()
  {
    // ImGui::BeginVertical("inputs", ImVec2(0, 0), 0.0f);
    ImGui::BeginVertical("inputs");

    NodeEd::PushStyleVar(NodeEd::StyleVar_PivotAlignment,
                         ImVec2(0, 0.5f));
    NodeEd::PushStyleVar(NodeEd::StyleVar_PivotSize, ImVec2(0, 0));

    for (Flode::Pin *i_Pin : pins) {
      if (i_Pin->direction == Flode::PinDirection::Input) {
        render_pin(i_Pin);
      }
    }

    NodeEd::PopStyleVar(2);
    ImGui::Spring(1, 0);
    ImGui::EndVertical();
  }

  void Node::render_output_pins()
  {
    ImGui::BeginVertical("outputs", ImVec2(0, 0), 1.0f);

    NodeEd::PushStyleVar(NodeEd::StyleVar_PivotAlignment,
                         ImVec2(1.0f, 0.5f));
    NodeEd::PushStyleVar(NodeEd::StyleVar_PivotSize, ImVec2(0, 0));

    for (Flode::Pin *i_Pin : pins) {
      if (i_Pin->direction == Flode::PinDirection::Output) {
        render_pin(i_Pin);
      }
    }

    NodeEd::PopStyleVar(2);

    ImGui::Spring(1, 0);
    ImGui::EndVertical();
  }

  void Node::default_render_compact()
  {
    NodeEd::GetStyle().NodeBorderWidth = 0.0f;
    NodeEd::GetStyle().NodePadding = g_NodePadding;

    /*
    NodeEd::PushStyleColor(
        NodeEd::StyleColor_NodeBg,
        color_to_imvec4(theme_get_current().header));
    NodeEd::PushStyleColor(NodeEd::StyleColor_Bg,
                           color_to_imvec4(theme_get_current().base));
    NodeEd::PushStyleColor(
        NodeEd::StyleColor_SelNodeBorder,
        color_to_imvec4(theme_get_current().button));
    NodeEd::PushStyleColor(
        NodeEd::StyleColor_PinRect,
        color_to_imvec4(theme_get_current().button));
                           */

    u32 l_ColorCount = 4;

    NodeEd::PushStyleVar(NodeEd::StyleVar_NodePadding, g_NodePadding);

    NodeEd::BeginNode(id);
    ImGui::BeginVertical("node");

    ImGui::PushID(id.AsPointer());

    ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.y * 2.0f);

    {
      ImGui::BeginHorizontal("content");
      ImGui::Spring(0, 0);
    }

    render_input_pins();

    ImGui::Spring(0);
    if (pins.size() == 1) {
      ImGui::PushFont(Low::Renderer::ImGuiHelper::fonts().common_500);
      ImGui::PushStyleColor(ImGuiCol_Text,
                            ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    } else {
      ImGui::PushFont(Low::Renderer::ImGuiHelper::fonts().common_800);
      ImGui::PushStyleColor(ImGuiCol_Text,
                            ImVec4(0.9f, 0.9f, 0.9f, 0.5f));
    }
    ImGui::TextUnformatted(get_name(NodeNameType::Compact).c_str());
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::Spring(0);

    render_output_pins();

    ImGui::EndHorizontal();
    ImGui::EndVertical();

    NodeEd::EndNode();

    NodeEd::PopStyleVar();

    ImGui::PopID();

    // NodeEd::PopStyleColor(l_ColorCount);
  }

  void Node::default_render()
  {
    NodeEd::GetStyle().NodeBorderWidth = 0.0f;
    NodeEd::GetStyle().NodePadding = g_NodePadding;

    /*
    NodeEd::PushStyleColor(
        NodeEd::StyleColor_NodeBg,
        color_to_imvec4(theme_get_current().header));
    NodeEd::PushStyleColor(NodeEd::StyleColor_Bg,
                           color_to_imvec4(theme_get_current().base));
    NodeEd::PushStyleColor(
        NodeEd::StyleColor_SelNodeBorder,
        color_to_imvec4(theme_get_current().button));
    NodeEd::PushStyleColor(
        NodeEd::StyleColor_PinRect,
        color_to_imvec4(theme_get_current().button));
                           */

    u32 l_ColorCount = 4;

    NodeEd::PushStyleVar(NodeEd::StyleVar_NodePadding, g_NodePadding);

    NodeEd::BeginNode(id);
    ImGui::BeginVertical("node");

    ImGui::PushID(id.AsPointer());

    render_header();

    ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.y * 2.0f);

    ImGui::BeginHorizontal("content");
    // ImGui::Spring(0, 0);

    render_input_pins();

    ImGui::Spring(1);

    render_output_pins();

    ImGui::EndHorizontal(); // End content

    ImGui::PopID();

    ImGui::EndVertical(); // End node

    NodeEd::EndNode();

    NodeEd::PopStyleVar();

    render_header_cosmetics();

    // ImGui::PopID();

    // NodeEd::PopStyleColor(l_ColorCount);
  }

  void Node::render()
  {
    if (is_compact()) {
      default_render_compact();
    } else {
      default_render();
    }
  }

  Variable *Graph::find_variable(Low::Util::String p_Name) const
  {
    for (Variable *i_Variable : m_Variables) {
      if (i_Variable->name == p_Name) {
        return i_Variable;
      }
    }

    return nullptr;
  }

  Node *Graph::find_node(NodeEd::NodeId p_NodeId) const
  {
    for (Flode::Node *i_Node : m_Nodes) {
      if (i_Node->id == p_NodeId) {
        return i_Node;
      }
    }

    return nullptr;
  }

  Pin *Graph::find_pin(NodeEd::PinId p_PinId) const
  {
    for (Flode::Node *i_Node : m_Nodes) {
      Flode::Pin *i_Pin = i_Node->find_pin(p_PinId);

      if (i_Pin) {
        return i_Pin;
      }
    }

    return nullptr;
  }

  void Graph::delete_node(NodeEd::NodeId p_NodeId)
  {
    for (auto it = m_Nodes.begin(); it != m_Nodes.end();) {
      Node *i_Node = *it;
      if (i_Node->id == p_NodeId) {
        it = m_Nodes.erase(it);

        delete i_Node;
      } else {
        ++it;
      }
    }

    clean_unconnected_links();
  }

  void Graph::delete_link(NodeEd::LinkId p_LinkId)
  {
    for (auto it = m_Links.begin(); it != m_Links.end();) {
      Link *i_Link = *it;
      if (i_Link->id == p_LinkId) {
        it = m_Links.erase(it);

        delete i_Link;
      } else {
        ++it;
      }
    }
  }

  Link *Graph::create_link_castable(NodeEd::PinId p_InputPin,
                                    NodeEd::PinId p_OutputPin)
  {
    Pin *l_InputPin = find_pin(p_InputPin);
    Pin *l_OutputPin = find_pin(p_OutputPin);

    _LOW_ASSERT(l_InputPin->direction == PinDirection::Input);
    _LOW_ASSERT(l_OutputPin->direction == PinDirection::Output);

    if (l_InputPin->type == PinType::Dynamic ||
        l_OutputPin->type == PinType::Dynamic) {
      return create_link(p_InputPin, p_OutputPin);
    }

    if (l_InputPin->type == l_OutputPin->type) {
      return create_link(p_InputPin, p_OutputPin);
    }

    LOW_ASSERT(can_cast(l_OutputPin->type, l_InputPin->type),
               "Cannot cast between these two pin types.");

    Low::Util::Name l_CastTypeName =
        get_cast_node_typename(l_OutputPin->type, l_InputPin->type);

    Node *l_CastNode = create_node(l_CastTypeName);

    create_link(p_InputPin, l_CastNode->pins[1]->id);

    return create_link(l_CastNode->pins[0]->id, p_OutputPin);
  }

  Link *Graph::create_link(NodeEd::PinId p_InputPin,
                           NodeEd::PinId p_OutputPin)
  {
    Pin *l_InputPin = find_pin(p_InputPin);
    Pin *l_OutputPin = find_pin(p_OutputPin);

    if (!l_OutputPin || !l_InputPin) {
      return nullptr;
    }

    // Disconnect links already at those pins if one of the pins does
    // not support multiple connections
    if (l_InputPin->type != PinType::Flow) {
      disconnect_pin(l_InputPin->id);
    }
    if (l_OutputPin->type == PinType::Flow) {
      disconnect_pin(l_OutputPin->id);
    }

    _LOW_ASSERT(l_InputPin->direction == PinDirection::Input);
    _LOW_ASSERT(l_OutputPin->direction == PinDirection::Output);

    Link *l_Link = new Link(NodeEd::LinkId(m_IdCounter++), p_InputPin,
                            p_OutputPin);
    m_Links.push_back(l_Link);

    find_node(l_InputPin->nodeId)->on_pin_connected(l_InputPin);
    find_node(l_OutputPin->nodeId)->on_pin_connected(l_OutputPin);

    // Draw new link
    /*
    NodeEd::Link(m_Links.back()->id, m_Links.back()->inputPinId,
                 m_Links.back()->outputPinId);
                 */

    return m_Links.back();
  }

  bool Graph::can_create_link(NodeEd::PinId p_InputPinId,
                              NodeEd::PinId p_OutputPinId)
  {
    if (p_InputPinId == p_OutputPinId) {
      return false;
    }

    Flode::Pin *l_InputPin = find_pin(p_InputPinId);
    Flode::Pin *l_OutputPin = find_pin(p_OutputPinId);

    if (l_InputPin->direction == l_OutputPin->direction) {
      return false;
    }

    if (l_InputPin->nodeId == l_OutputPin->nodeId) {
      return false;
    }

    // Both pins can't be dynamic
    if (l_InputPin->type == PinType::Dynamic &&
        l_OutputPin->type == PinType::Dynamic) {
      return false;
    }

    if (l_InputPin->type == PinType::Dynamic) {
      Node *l_Node = find_node(l_InputPin->nodeId);

      return l_Node->accept_dynamic_pin_connection(l_InputPin,
                                                   l_OutputPin);
    }

    if (l_OutputPin->type == PinType::Dynamic) {
      Node *l_Node = find_node(l_OutputPin->nodeId);

      return l_Node->accept_dynamic_pin_connection(l_OutputPin,
                                                   l_InputPin);
    }

    if (l_InputPin->type != l_OutputPin->type) {
      if (!can_cast(l_OutputPin->type, l_InputPin->type)) {
        return false;
      }
    }

    return true;
  }

  Node *Graph::create_node(Low::Util::Name p_TypeName,
                           bool p_SetupPins)
  {
    Node *l_Node = spawn_node_of_type(p_TypeName);

    l_Node->graph = this;

    l_Node->id = m_IdCounter++;

    l_Node->initialize();

    if (p_SetupPins) {
      l_Node->setup_default_pins();
    }

    m_Nodes.push_back(l_Node);

    return l_Node;
  }

  NodeEd::PinId Graph::get_connected_pin(NodeEd::PinId p_PinId) const
  {
    for (Link *i_Link : m_Links) {
      if (i_Link->inputPinId == p_PinId) {
        return i_Link->outputPinId;
      }
      if (i_Link->outputPinId == p_PinId) {
        return i_Link->inputPinId;
      }
    }

    _LOW_ASSERT(false);

    return 0;
  }

  void Graph::disconnect_pin(NodeEd::PinId p_PinId)
  {
    Low::Util::List<Link *> l_LinksToDelete;
    for (Link *i_Link : m_Links) {
      if (i_Link->inputPinId == p_PinId) {
        l_LinksToDelete.push_back(i_Link);
      }
      if (i_Link->outputPinId == p_PinId) {
        l_LinksToDelete.push_back(i_Link);
      }
    }

    for (auto it = l_LinksToDelete.begin();
         it != l_LinksToDelete.end();) {
      Link *i_Link = *it;
      delete_link(i_Link->id);
      it = l_LinksToDelete.erase(it);
    }
  }

  bool Graph::is_pin_connected(NodeEd::PinId p_PinId) const
  {
    // TODO: Make more efficient by caching info

    for (Link *i_Link : m_Links) {
      if (i_Link->inputPinId == p_PinId) {
        return true;
      }
      if (i_Link->outputPinId == p_PinId) {
        return true;
      }
    }

    return false;
  }

  void Graph::clean_unconnected_links()
  {
    for (auto it = m_Links.begin(); it != m_Links.end();) {
      Link *i_Link = *it;

      Pin *i_InputPin = find_pin(i_Link->inputPinId);
      Pin *i_OutputPin = find_pin(i_Link->outputPinId);

      if (i_InputPin && i_OutputPin) {
        ++it;
      } else {
        it = m_Links.erase(it);
        delete i_Link;
      }
    }
  }

  u64 Graph::deserialize_node(Low::Util::Yaml::Node &p_Yaml,
                              Node **p_Node, u64 p_IdStartValue,
                              bool p_UseIds)
  {
    Low::Util::Name l_NodeTypeName = LOW_YAML_AS_NAME(p_Yaml["type"]);

    u64 l_IdsAdded = 0;

    *p_Node = spawn_node_of_type(l_NodeTypeName);
    Node *l_Node = *p_Node;
    if (p_UseIds) {
      l_Node->id = p_Yaml["id"].as<u64>();
    } else {
      l_Node->id = p_IdStartValue + l_IdsAdded;
      l_IdsAdded++;
    }
    l_Node->graph = this;
    l_Node->initialize();

    if (p_Yaml["node_data"]) {
      l_Node->deserialize(p_Yaml["node_data"]);
    }

    l_Node->setup_default_pins();

    for (u32 i = 0; i < l_Node->pins.size(); ++i) {
      if (p_UseIds && p_Yaml["pins"].size() > i) {
        l_Node->pins[i]->id = p_Yaml["pins"][i]["id"].as<u64>();
      } else {
        l_Node->pins[i]->id = p_IdStartValue + l_IdsAdded;
        l_IdsAdded++;
      }
      if (p_Yaml["pins"][i]["default_value"]) {
        l_Node->pins[i]->defaultValue =
            Low::Util::Serialization::deserialize_variant(
                p_Yaml["pins"][i]["default_value"]);
      }
      if (p_Yaml["pins"][i]["default_string_value"]) {
        l_Node->pins[i]->defaultStringValue = LOW_YAML_AS_STRING(
            p_Yaml["pins"][i]["default_string_value"]);

        if (l_Node->pins[i]->stringType == PinStringType::Name) {
          // In case the pin is actually a name convert the read
          // string default value to a name
          l_Node->pins[i]->defaultValue =
              Low::Util::Name::from_string(
                  l_Node->pins[i]->defaultStringValue);
        }
      }
    }

    Low::Math::Vector2 l_Position =
        Low::Util::Serialization::deserialize_vector2(
            p_Yaml["position"]);
    NodeEd::SetNodePosition(l_Node->id,
                            ImVec2(l_Position.x, l_Position.y));

    return l_IdsAdded;
  }

  void Graph::serialize_node(const Node *p_Node,
                             Low::Util::Yaml::Node &p_Yaml,
                             bool p_StoreNodePositions) const
  {
    p_Yaml["type"] = p_Node->typeName.c_str();
    p_Yaml["id"] = p_Node->id.Get();

    for (Pin *i_Pin : p_Node->pins) {
      Low::Util::Yaml::Node i_PinYaml;
      i_PinYaml["id"] = i_Pin->id.Get();
      i_PinYaml["direction"] =
          pin_direction_to_string(i_Pin->direction).c_str();
      i_PinYaml["type"] = pin_type_to_string(i_Pin->type).c_str();
      i_PinYaml["type_id"] = i_Pin->typeId;

      if (i_Pin->type == PinType::String) {
        if (i_Pin->stringType == PinStringType::Name) {
          Low::Util::Name i_NameContent = i_Pin->defaultValue;
          i_PinYaml["default_string_value"] = i_NameContent.c_str();
        } else {
          i_PinYaml["default_string_value"] =
              i_Pin->defaultStringValue.c_str();
        }
      } else {
        if (i_Pin->defaultValue.m_Type !=
                Low::Util::VariantType::String &&
            i_Pin->defaultValue.m_Type !=
                Low::Util::VariantType::Handle) {
          Low::Util::Serialization::serialize_variant(
              i_PinYaml["default_value"], i_Pin->defaultValue);
        }
      }

      p_Yaml["pins"].push_back(i_PinYaml);
    }

    Low::Math::Vector2 l_Position;
    if (p_StoreNodePositions) {
      ImVec2 l_ImPos = NodeEd::GetNodePosition(p_Node->id);
      l_Position.x = l_ImPos.x;
      l_Position.y = l_ImPos.y;
    } else {
      l_Position.x = 0.0f;
      l_Position.y = 0.0f;
    }

    Low::Util::Serialization::serialize(p_Yaml["position"],
                                        l_Position);

    p_Node->serialize(p_Yaml["node_data"]);
  }

  void Graph::serialize(Low::Util::Yaml::Node &p_Node,
                        bool p_StoreNodePositions) const
  {
    p_Node["name"] = m_Name.c_str();
    p_Node["idcounter"] = m_IdCounter;
    p_Node["internal"] = m_Internal;

    Low::Util::Serialization::serialize_handle(
        p_Node["contexthandle"], m_ContextHandle);

    for (auto it = m_Namespace.begin(); it != m_Namespace.end();
         ++it) {
      // Low::Util::Yaml::Node i_Node;
      p_Node["namespace"].push_back(it->c_str());
    }

    for (const Variable *i_Variable : m_Variables) {
      Low::Util::Yaml::Node i_Yaml;

      i_Yaml["name"] = i_Variable->name.c_str();
      i_Yaml["type"] = pin_type_to_string(i_Variable->type).c_str();
      i_Yaml["type_id"] = i_Variable->typeId;

      p_Node["variables"].push_back(i_Yaml);
    }

    for (const Node *i_Node : m_Nodes) {
      Low::Util::Yaml::Node i_Yaml;

      serialize_node(i_Node, i_Yaml, p_StoreNodePositions);

      p_Node["nodes"].push_back(i_Yaml);
    }

    for (const Link *i_Link : m_Links) {
      Low::Util::Yaml::Node i_LinkYaml;
      i_LinkYaml["id"] = i_Link->id.Get();
      i_LinkYaml["input"] = i_Link->inputPinId.Get();
      i_LinkYaml["output"] = i_Link->outputPinId.Get();

      p_Node["links"].push_back(i_LinkYaml);
    }
  }

  void Graph::deserialize(Low::Util::Yaml::Node &p_Node)
  {
    m_Name = LOW_YAML_AS_NAME(p_Node["name"]);

    if (p_Node["contexthandle"]) {
      m_ContextHandle = Low::Util::Serialization::deserialize_handle(
          p_Node["contexthandle"]);
    }

    const char *l_NamespaceName = "namespace";

    u32 l_IdsAdded = 0;

    if (p_Node[l_NamespaceName]) {
      for (auto it = p_Node[l_NamespaceName].begin();
           it != p_Node[l_NamespaceName].end(); ++it) {
        Low::Util::Yaml::Node &i_Node = *it;
        m_Namespace.push_back(LOW_YAML_AS_NAME(i_Node));
      }
    }

    if (p_Node["internal"]) {
      m_Internal = p_Node["internal"].as<bool>();
    }

    if (p_Node["variables"]) {
      for (auto it = p_Node["variables"].begin();
           it != p_Node["variables"].end(); ++it) {
        Low::Util::Yaml::Node &i_VariableNode = *it;

        Variable *i_Variable = new Variable;
        i_Variable->name = LOW_YAML_AS_STRING(i_VariableNode["name"]);
        i_Variable->type = string_to_pin_type(
            LOW_YAML_AS_STRING(i_VariableNode["type"]));
        if (i_VariableNode["type_id"]) {
          i_Variable->typeId = i_VariableNode["type_id"].as<u16>();
        } else {
          i_Variable->typeId = 0;
        }

        m_Variables.push_back(i_Variable);
      }
    }

    if (p_Node["nodes"]) {
      for (auto it = p_Node["nodes"].begin();
           it != p_Node["nodes"].end(); ++it) {
        Low::Util::Yaml::Node &i_NodeNode = *it;

        Node *i_Node = nullptr;
        l_IdsAdded += deserialize_node(
            i_NodeNode, &i_Node,
            i_NodeNode["id"].as<u64>() + l_IdsAdded, true);

        if (!i_Node) {
          continue;
        }
        m_Nodes.push_back(i_Node);
      }
    }

    if (p_Node["links"]) {
      for (auto it = p_Node["links"].begin();
           it != p_Node["links"].end(); ++it) {
        Low::Util::Yaml::Node &i_LinkNode = *it;

        Link *i_Link = create_link(i_LinkNode["input"].as<u64>(),
                                   i_LinkNode["output"].as<u64>());

        if (!i_Link) {
          continue;
        }

        i_Link->id = i_LinkNode["id"].as<u64>();
      }
    }

    // I think it is important to do that last so that all the ids
    // are synced up again
    if (p_Node["idcounter"]) {
      m_IdCounter = p_Node["idcounter"].as<u64>() + l_IdsAdded;
    }
  }

  void Graph::compile() const
  {
    Low::Util::StringBuilder l_Builder;

    l_Builder.append("#pragma once").endl().endl();
    l_Builder.append("#include \"LowUtil.h\"").endl();
    l_Builder.append("#include \"LowUtilString.h\"").endl();
    l_Builder.append("#include \"LowUtilContainers.h\"").endl();
    l_Builder.append("#include \"LowUtilLogger.h\"").endl();
    l_Builder.endl();
    for (auto it = m_Namespace.begin(); it != m_Namespace.end();
         ++it) {
      l_Builder.append("namespace ").append(*it).append(" {").endl();
    }

    for (Variable *i_Variable : m_Variables) {
      switch (i_Variable->type) {
      case PinType::Number: {
        l_Builder.append("float");
        break;
      }
      case PinType::Bool: {
        l_Builder.append("bool");
        break;
      }
      case PinType::String: {
        l_Builder.append("Low::Util::String");
        break;
      }
      case PinType::Handle: {
        if (i_Variable->typeId == 0) {
          l_Builder.append("Low::Util::Handle");
        } else {
          Low::Editor::TypeMetadata &i_Metadata =
              Low::Editor::get_type_metadata(i_Variable->typeId);
          l_Builder.append(i_Metadata.fullTypeString);
        }
        break;
      }
      }

      l_Builder.append(" ").append(i_Variable->name);
      if (i_Variable->type == PinType::Handle) {
        l_Builder.append("(0)");
      } else {
        l_Builder.append(" = ");
        switch (i_Variable->type) {
        case PinType::Number: {
          l_Builder.append("0.0f");
          break;
        }
        case PinType::Bool: {
          l_Builder.append("false");
          break;
        }
        case PinType::String: {
          l_Builder.append("Low::Util::String(\"\")");
          break;
        }
        }
      }
      l_Builder.append(";").endl();
    }

    for (Node *i_Node : m_Nodes) {
      if (i_Node->typeName == N(FlodeSyntaxFunction)) {
        i_Node->compile(l_Builder);
      }
    }

    for (auto it = m_Namespace.begin(); it != m_Namespace.end();
         ++it) {
      l_Builder.append("}").endl();
    }

    Low::Util::String l_Path = Low::Util::get_project().dataPath;
    l_Path +=
        "/scripts/" + Low::Util::String(m_Name.c_str()) + ".cpp";

    Low::Util::FileIO::File l_File = Low::Util::FileIO::open(
        l_Path.c_str(), Low::Util::FileIO::FileMode::WRITE);

    Low::Util::FileIO::write_sync(l_File, l_Builder.get().c_str());

    Low::Util::FileIO::close(l_File);

    LOW_LOG_INFO << "Compiled flode graph '"
                 << "test"
                 << "' to file." << LOW_LOG_END;
  }

  void
  Graph::continue_compilation(Low::Util::StringBuilder &p_Builder,
                              Pin *p_Pin) const
  {
    _LOW_ASSERT(p_Pin);
    _LOW_ASSERT(p_Pin->direction == PinDirection::Output);
    _LOW_ASSERT(p_Pin->type == PinType::Flow);

    if (is_pin_connected(p_Pin->id)) {
      NodeEd::PinId l_ConnectedFlowPinId =
          get_connected_pin(p_Pin->id);

      Pin *l_ConnectedFlowPin = find_pin(l_ConnectedFlowPinId);
      Node *l_ConnectedNode = find_node(l_ConnectedFlowPin->nodeId);

      l_ConnectedNode->compile(p_Builder);
    }
  }

  bool Graph::has_context_handle() const
  {
    if (!m_ContextHandle.is_registered_type()) {
      return false;
    }

    Low::Util::RTTI::TypeInfo &l_TypeInfo =
        Low::Util::Handle::get_type_info(m_ContextHandle.get_type());

    return l_TypeInfo.is_alive(m_ContextHandle);
  }

  u16 Graph::get_context_handle_type_id() const
  {
    if (!has_context_handle()) {
      return 0;
    }

    return m_ContextHandle.get_type();
  }
} // namespace Flode
