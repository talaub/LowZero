#include "FlodeSyntaxNodes.h"
#include <cstring>

#include "LowEditor.h"
#include "LowEditorBase.h"
#include "LowEditorMainWindow.h"

#include "IconsFontAwesome5.h"
#include "IconsLucide.h"

namespace Flode {
  namespace SyntaxNodes {

    ImU32 g_SyntaxColor = IM_COL32(138, 46, 119, 255);

    FunctionNode::FunctionNode()
    {
      m_Name = N(FunctionName);
    }

    Low::Util::String
    FunctionNode::get_name(NodeNameType p_Type) const
    {
      return m_Name.c_str();
    }

    Low::Util::String
    FunctionNode::get_subtitle(NodeNameType p_Type) const
    {
      return "Function";
    }

    ImU32 FunctionNode::get_color() const
    {
      return g_SyntaxColor;
    }

    void FunctionNode::setup_default_pins()
    {
      // create_pin(PinDirection::Input, "", PinType::Flow);
      create_pin(PinDirection::Output, "", PinType::Flow);

      create_dynamic_pins();
    }

    void FunctionNode::create_dynamic_pins()
    {
      int i = 0;
      for (auto it = pins.begin(); it != pins.end();) {
        Pin *i_Pin = *it;
        if (i == 0) {
          it++;
        } else {
          it = pins.erase(it);
          delete i_Pin;
        }

        i++;
      }

      for (auto it = m_Parameters.begin(); it != m_Parameters.end();
           ++it) {
        FunctionNodeParameter &i_Param = *it;

        if (i_Param.type == PinType::Handle) {
          Pin *i_Pin = create_handle_pin(
              PinDirection::Output, i_Param.name.c_str(),
              i_Param.typeId, i_Param.pinId.Get());

          i_Param.pinId = i_Pin->id;
        } else if (i_Param.type == PinType::Enum) {
          Pin *i_Pin = create_enum_pin(
              PinDirection::Output, i_Param.name.c_str(),
              i_Param.typeId, i_Param.pinId.Get());

          i_Param.pinId = i_Pin->id;
        } else {
          Pin *i_Pin =
              create_pin(PinDirection::Output, i_Param.name.c_str(),
                         i_Param.type, i_Param.pinId.Get());

          i_Param.pinId = i_Pin->id;
        }
      }

      graph->clean_unconnected_links();
    }

    void FunctionNode::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      p_Node["name"] = m_Name.c_str();
      p_Node["editable"] = m_Editable;
      p_Node["hasreturnvalue"] = m_HasReturnValue;
      if (m_HasReturnValue) {
        p_Node["returntype"] =
            pin_type_to_string(m_ReturnType).c_str();
        p_Node["returntypeid"] = m_ReturnTypeId;
      }
      for (auto it = m_Parameters.begin(); it != m_Parameters.end();
           ++it) {
        Low::Util::Yaml::Node i_ParamNode;

        i_ParamNode["name"] = it->name.c_str();
        i_ParamNode["type"] = pin_type_to_string(it->type).c_str();
        i_ParamNode["pin_id"] = it->pinId.Get();
        i_ParamNode["type_id"] = it->typeId;

        p_Node["parameters"].push_back(i_ParamNode);
      }
    }

    void FunctionNode::deserialize(Low::Util::Yaml::Node &p_Node)
    {
      m_Name = N(def_func);
      if (p_Node["name"]) {
        m_Name = LOW_YAML_AS_NAME(p_Node["name"]);
      }

      if (p_Node["editable"]) {
        m_Editable = p_Node["editable"].as<bool>();
      } else {
        m_Editable = true;
      }
      m_HasReturnValue = false;
      m_ReturnType = PinType::Number;
      m_ReturnTypeId = 0;

      if (p_Node["hasreturnvalue"]) {
        m_HasReturnValue = p_Node["hasreturnvalue"].as<bool>();
      }
      if (p_Node["returntype"]) {
        m_ReturnType = string_to_pin_type(
            LOW_YAML_AS_STRING(p_Node["returntype"]));
      }
      if (p_Node["returntypeid"]) {
        m_ReturnTypeId = p_Node["returntypeid"].as<u16>();
      }

      if (p_Node["parameters"]) {
        for (auto it = p_Node["parameters"].begin();
             it != p_Node["parameters"].end(); ++it) {
          Low::Util::Yaml::Node &i_Node = *it;

          FunctionNodeParameter i_Param;
          i_Param.name = LOW_YAML_AS_NAME(i_Node["name"]);
          i_Param.type =
              string_to_pin_type(LOW_YAML_AS_STRING(i_Node["type"]));
          i_Param.pinId = i_Node["pin_id"].as<u64>();
          i_Param.typeId = 0;
          if (i_Node["type_id"]) {
            i_Param.typeId = i_Node["type_id"].as<u16>();
          }

          m_Parameters.push_back(i_Param);
        }
      }
    }

    void FunctionNode::render_data()
    {
      if (!m_Editable) {
        return;
      }
      ImGui::Text("Name");
      ImGui::SameLine();
      ImGui::PushItemWidth(120.0f);
      Low::Editor::Base::NameEdit("##nameedit", &m_Name);
      ImGui::PopItemWidth();

      ImGui::Text("Parameters");

      bool l_Changed = false;

      if (ImGui::Button(ICON_LC_PLUS "")) {
        FunctionNodeParameter l_Param;
        l_Param.name = "new";
        l_Param.type = PinType::Number;
        l_Param.pinId = 0;

        m_Parameters.push_back(l_Param);

        l_Changed = true;
      }

      int l_ParamCount = 0;

      Low::Util::List<PinType> l_Types = {
          PinType::String, PinType::Number, PinType::Bool,
          PinType::Handle};

      struct Funcs
      {
        static bool PinTypeGetter(void *data, int n,
                                  const char **out_str)
        {
          PinType l_Type = ((PinType *)data)[n];
          Low::Util::String l_Name = pin_type_to_string(l_Type);
          *out_str = (char *)malloc(l_Name.size() + 1);
          memcpy((void *)*out_str, l_Name.c_str(), l_Name.size());
          (*(char **)out_str)[l_Name.size()] = '\0';
          return true;
        }
      };

      for (auto it = m_Parameters.begin();
           it != m_Parameters.end();) {

        ImGui::PushID(it->pinId.AsPointer());

        FunctionNodeParameter &i_Parameter = *it;

        char l_Buffer[255];
        uint32_t l_NameLength = strlen(it->name.c_str());
        memcpy(l_Buffer, it->name.c_str(), l_NameLength);
        l_Buffer[l_NameLength] = '\0';

        ImGui::PushItemWidth(100.0f);
        if (ImGui::InputText("##paramname", l_Buffer, 255,
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
          Low::Util::Name i_Val = LOW_NAME(l_Buffer);
          i_Parameter.name = i_Val;
          l_Changed = true;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        int l_CurrentValue = 0;
        for (int i = 0; i < l_Types.size(); ++i) {
          if (l_Types[i] == i_Parameter.type) {
            l_CurrentValue = i;
            break;
          }
        }

        ImGui::PushItemWidth(100.0f);
        bool l_Result = ImGui::Combo("##paramtype", &l_CurrentValue,
                                     &Funcs::PinTypeGetter,
                                     l_Types.data(), l_Types.size());
        ImGui::PopItemWidth();

        bool i_Deleted = false;

        if (l_Result) {
          l_Changed = true;

          // i_Parameter.type = static_cast<PinType>(l_CurrentValue);
          i_Parameter.type = l_Types[l_CurrentValue];
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_LC_TRASH_2 "")) {
          it = m_Parameters.erase(it);
          l_Changed = true;
          i_Deleted = true;
        } else {
        }

        if (!i_Deleted) {
          if (it->type == PinType::Handle) {
            ImGui::Dummy(ImVec2(100.0f, 0.0f));
            ImGui::SameLine();

            int i_CurrentTypeValue = 0;

            for (; i_CurrentTypeValue < get_exposed_types().size();
                 ++i_CurrentTypeValue) {
              if (get_exposed_types()[i_CurrentTypeValue].typeId ==
                  it->typeId) {
                break;
              }
            }

            ImGui::PushItemWidth(100.0f);
            bool i_TypeChanged = ImGui::Combo(
                "##variableselector", &i_CurrentTypeValue,
                [](void *data, int n, const char **out_str) {
                  Low::Editor::TypeMetadata &l_Metadata =
                      ((Low::Editor::TypeMetadata *)data)[n];
                  Low::Util::String l_Name = l_Metadata.name.c_str();
                  *out_str = (char *)malloc(l_Name.size() + 1);
                  memcpy((void *)*out_str, l_Name.c_str(),
                         l_Name.size());
                  (*(char **)out_str)[l_Name.size()] = '\0';
                  return true;
                },
                get_exposed_types().data(),
                get_exposed_types().size());
            ImGui::PopItemWidth();

            if (i_TypeChanged) {
              for (u32 i = 0; i < get_exposed_types().size(); ++i) {
                if (i == i_CurrentTypeValue) {
                  it->typeId = get_exposed_types()[i].typeId;
                  l_Changed = true;
                  break;
                }
              }
            }
          }
        }

        if (i_Deleted) {
          it = m_Parameters.erase(it);
        } else {
          it++;
        }

        l_ParamCount++;

        ImGui::PopID();
      }

      if (l_Changed) {
        create_dynamic_pins();
      }
    }

    static Low::Util::String pin_type_to_cpp_string(PinType p_Type)
    {
      switch (p_Type) {
      case PinType::String:
        return "Low::Util::String";
      case PinType::Number:
        return "float";
      case PinType::Bool:
        return "bool";
      }

      _LOW_ASSERT(false);
    }

    void
    FunctionNode::compile(Low::Util::StringBuilder &p_Builder) const
    {
      if (!m_HasReturnValue) {
        p_Builder.append("void ");
      } else {
        p_Builder.append(pin_type_to_cpp_string(m_ReturnType))
            .append(" ");
      }
      p_Builder.append(m_Name);
      p_Builder.append("(");

      int l_ParameterCount = 0;
      for (auto it = m_Parameters.begin(); it != m_Parameters.end();
           ++it) {
        if (l_ParameterCount) {
          p_Builder.append(", ");
        }

        if (it->type == PinType::Handle) {
          if (it->typeId == 0) {
            p_Builder.append("Low::Util::Handle");
          } else {
            Low::Editor::TypeMetadata &i_Metadata =
                Low::Editor::get_type_metadata(it->typeId);
            p_Builder.append(i_Metadata.fullTypeString);
          }
        } else if (it->type == PinType::Enum) {
          Low::Editor::EnumMetadata &i_Metadata =
              Low::Editor::get_enum_metadata(
                  Low::Util::get_enum_info(it->typeId).name);
          p_Builder.append(i_Metadata.fullTypeString);
        } else {
          p_Builder.append(pin_type_to_cpp_string(it->type));
        }
        p_Builder.append(" ");

        p_Builder.append("p_");
        p_Builder.append(it->name);

        l_ParameterCount++;
      }

      p_Builder.append(") {\n");

      graph->continue_compilation(p_Builder, pins[0]);

      p_Builder.append("}\n");
    }

    void FunctionNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_pin_checked(p_PinId);

      LOW_ASSERT(l_Pin->direction == PinDirection::Output,
                 "Pin is not an output pin");

      bool l_Found = false;
      for (auto it = m_Parameters.begin(); it != m_Parameters.end();
           ++it) {
        if (it->pinId == l_Pin->id) {
          p_Builder.append("p_");
          p_Builder.append(it->name);
          l_Found = true;
          break;
        }
      }

      _LOW_ASSERT(l_Found);
    }

    Node *function_create_instance()
    {
      FunctionNode *l_Node = new FunctionNode;
      l_Node->m_Editable = true;
      l_Node->m_HasReturnValue = false;
      return l_Node;
    }

    GetVariableNode::GetVariableNode() : m_Variable(nullptr)
    {
    }

    void GetVariableNode::set_variable(Variable *p_Variable)
    {
      m_Variable = p_Variable;
      create_dynamic_pins();
    }

    Low::Util::String
    GetVariableNode::get_name(NodeNameType p_Type) const
    {
      if (m_Variable) {
        return m_Variable->name;
      }
      return "Get variable";
    }

    ImU32 GetVariableNode::get_color() const
    {
      return g_SyntaxColor;
    }

    void GetVariableNode::setup_default_pins()
    {
      create_dynamic_pins();
    }

    void GetVariableNode::create_dynamic_pins()
    {
      // Clear all pins
      for (auto it = pins.begin(); it != pins.end();) {
        Pin *i_Pin = *it;
        it = pins.erase(it);
        delete i_Pin;
      }

      if (m_Variable) {
        Pin *l_Pin =
            create_pin(PinDirection::Output, "", m_Variable->type);
        if (m_Variable->type == PinType::String) {
          l_Pin->stringType = Flode::PinStringType::String;
        }
      }

      graph->clean_unconnected_links();
    }

    void
    GetVariableNode::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      if (m_Variable) {
        p_Node["variable"] = m_Variable->name.c_str();
      }
    }

    void GetVariableNode::deserialize(Low::Util::Yaml::Node &p_Node)
    {
      m_Variable = nullptr;
      if (p_Node["variable"]) {
        m_Variable = graph->find_variable(
            LOW_YAML_AS_STRING(p_Node["variable"]));
      }
    }

    void GetVariableNode::render_data()
    {
      ImGui::Text("Variable");
      ImGui::PushItemWidth(200.0f);

      int l_CurrentValue = 0;
      for (; l_CurrentValue < graph->m_Variables.size();
           ++l_CurrentValue) {
        if (graph->m_Variables[l_CurrentValue] == m_Variable) {
          break;
        }
      }

      bool l_Result = ImGui::Combo(
          "##variableselector", &l_CurrentValue,
          [](void *data, int n, const char **out_str) {
            Variable *l_Variable = ((Variable **)data)[n];
            Low::Util::String l_Name = l_Variable->name;
            *out_str = (char *)malloc(l_Name.size() + 1);
            memcpy((void *)*out_str, l_Name.c_str(), l_Name.size());
            (*(char **)out_str)[l_Name.size()] = '\0';
            return true;
          },
          graph->m_Variables.data(), graph->m_Variables.size());

      if (l_Result) {
        m_Variable = graph->m_Variables[l_CurrentValue];
        create_dynamic_pins();
      }
      ImGui::PopItemWidth();
    }

    void GetVariableNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      LOW_ASSERT(m_Variable, "No variable selected");

      if (m_Variable) {
        p_Builder.append(m_Variable->name);
      }
    }

    Node *getvariable_create_instance()
    {
      return new GetVariableNode;
    }

    SetVariableNode::SetVariableNode() : m_Variable(nullptr)
    {
    }

    Low::Util::String
    SetVariableNode::get_name(NodeNameType p_Type) const
    {
      if (m_Variable) {
        Low::Util::String l_Title = "Set ";
        l_Title += m_Variable->name;
        return l_Title;
      }
      return "Set variable";
    }

    ImU32 SetVariableNode::get_color() const
    {
      return g_SyntaxColor;
    }

    void SetVariableNode::set_variable(Variable *p_Variable)
    {
      m_Variable = p_Variable;
      create_dynamic_pins();
    }

    void SetVariableNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Flow);
      create_pin(PinDirection::Output, "", PinType::Flow);

      create_dynamic_pins();
    }

    void SetVariableNode::create_dynamic_pins()
    {
      int i = 0;
      for (auto it = pins.begin(); it != pins.end();) {
        // Skip the first two pins because they're the flow pins
        if (i < 2) {
          it++;
          continue;
        }
        Pin *i_Pin = *it;
        it = pins.erase(it);
        delete i_Pin;
      }

      if (m_Variable) {
        create_pin(PinDirection::Input, m_Variable->name,
                   m_Variable->type);
      }

      graph->clean_unconnected_links();
    }

    void
    SetVariableNode::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      if (m_Variable) {
        p_Node["variable"] = m_Variable->name.c_str();
      }
    }

    void SetVariableNode::deserialize(Low::Util::Yaml::Node &p_Node)
    {
      m_Variable = nullptr;
      if (p_Node["variable"]) {
        m_Variable = graph->find_variable(
            LOW_YAML_AS_STRING(p_Node["variable"]));
      }
    }

    void SetVariableNode::render_data()
    {
      ImGui::Text("Variable");
      ImGui::PushItemWidth(200.0f);

      int l_CurrentValue = 0;
      for (; l_CurrentValue < graph->m_Variables.size();
           ++l_CurrentValue) {
        if (graph->m_Variables[l_CurrentValue] == m_Variable) {
          break;
        }
      }

      bool l_Result = ImGui::Combo(
          "##variableselector", &l_CurrentValue,
          [](void *data, int n, const char **out_str) {
            Variable *l_Variable = ((Variable **)data)[n];
            Low::Util::String l_Name = l_Variable->name;
            *out_str = (char *)malloc(l_Name.size() + 1);
            memcpy((void *)*out_str, l_Name.c_str(), l_Name.size());
            (*(char **)out_str)[l_Name.size()] = '\0';
            return true;
          },
          graph->m_Variables.data(), graph->m_Variables.size());

      if (l_Result) {
        m_Variable = graph->m_Variables[l_CurrentValue];
        create_dynamic_pins();
      }
      ImGui::PopItemWidth();
    }

    void SetVariableNode::compile(
        Low::Util::StringBuilder &p_Builder) const
    {
      LOW_ASSERT(m_Variable,
                 "Needs to have variable set on set variable node");

      p_Builder.append(m_Variable->name);
      p_Builder.append(" = ");
      compile_input_pin(p_Builder,
                        pins[2]->id); // Compile the value input pin
      p_Builder.append(";").endl();

      graph->continue_compilation(
          p_Builder,
          pins[1]); // Continue compilation at flow output pin
    }

    Node *setvariable_create_instance()
    {
      return new SetVariableNode;
    }

    ReturnNumberNode::ReturnNumberNode()
    {
    }

    Low::Util::String
    ReturnNumberNode::get_name(NodeNameType p_Type) const
    {
      return "Return";
    }

    ImU32 ReturnNumberNode::get_color() const
    {
      return g_SyntaxColor;
    }

    void ReturnNumberNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Flow);
      create_pin(PinDirection::Input, "", PinType::Number);
    }

    void ReturnNumberNode::compile(
        Low::Util::StringBuilder &p_Builder) const
    {
      p_Builder.append("return ");
      compile_input_pin(p_Builder,
                        pins[1]->id); // Compile the value input pin
      p_Builder.append(";").endl();
    }

    Node *returnnumber_create_instance()
    {
      return new ReturnNumberNode;
    }

    ReturnBoolNode::ReturnBoolNode()
    {
    }

    Low::Util::String
    ReturnBoolNode::get_name(NodeNameType p_Type) const
    {
      return "Return";
    }

    ImU32 ReturnBoolNode::get_color() const
    {
      return g_SyntaxColor;
    }

    void ReturnBoolNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Flow);
      create_pin(PinDirection::Input, "", PinType::Bool);
    }

    void
    ReturnBoolNode::compile(Low::Util::StringBuilder &p_Builder) const
    {
      p_Builder.append("return ");
      compile_input_pin(p_Builder,
                        pins[1]->id); // Compile the value input pin
      p_Builder.append(";").endl();
    }

    Node *returnbool_create_instance()
    {
      return new ReturnBoolNode;
    }

    Low::Util::String IfNode::get_name(NodeNameType p_Type) const
    {
      return "if";
    }

    ImU32 IfNode::get_color() const
    {
      return g_SyntaxColor;
    }

    void IfNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Flow);
      create_pin(PinDirection::Input, "", PinType::Bool);
      create_pin(PinDirection::Output, "True", PinType::Flow);
      create_pin(PinDirection::Output, "False", PinType::Flow);
    }

    void IfNode::compile(Low::Util::StringBuilder &p_Builder) const
    {
      if (graph->is_pin_connected(pins[2]->id)) {
        p_Builder.append("if (");
        compile_input_pin(p_Builder, pins[1]->id);
        p_Builder.append(") {").endl();
        graph->continue_compilation(p_Builder, pins[2]);
        p_Builder.append("}").endl();
        if (graph->is_pin_connected(pins[3]->id)) {
          p_Builder.append("else {").endl();
          graph->continue_compilation(p_Builder, pins[3]);
          p_Builder.append("}").endl();
        }
      } else if (graph->is_pin_connected(pins[3]->id)) {
        p_Builder.append("if (!(");
        compile_input_pin(p_Builder, pins[1]->id);
        p_Builder.append(")) {").endl();
        graph->continue_compilation(p_Builder, pins[3]);
        p_Builder.append("}").endl();
      }
    }

    Low::Util::String ForNode::get_name(NodeNameType p_Type) const
    {
      return "for";
    }

    ImU32 ForNode::get_color() const
    {
      return g_SyntaxColor;
    }

    void ForNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Flow);
      create_pin(PinDirection::Input, "Start", PinType::Number);
      create_pin(PinDirection::Input, "End", PinType::Number);
      create_pin(PinDirection::Input, "Step", PinType::Number);
      create_pin(PinDirection::Output, "", PinType::Flow);
      create_pin(PinDirection::Output, "Loop", PinType::Flow);
      create_pin(PinDirection::Output, "Index", PinType::Number);
    }

    void ForNode::compile(Low::Util::StringBuilder &p_Builder) const
    {
      Low::Util::StringBuilder l_IndexVariableName;
      l_IndexVariableName.append("__for_index").append(id.Get());

      p_Builder.append("for (int ")
          .append(l_IndexVariableName.get())
          .append(" = ");
      compile_input_pin(p_Builder, pins[1]->id);
      p_Builder.append("; ")
          .append(l_IndexVariableName.get())
          .append(" < ");
      compile_input_pin(p_Builder, pins[2]->id);
      p_Builder.append("; ")
          .append(l_IndexVariableName.get())
          .append(" += ");
      compile_input_pin(p_Builder, pins[3]->id);
      p_Builder.append(") {").endl();
      graph->continue_compilation(p_Builder, pins[5]);
      p_Builder.append("}").endl();

      graph->continue_compilation(p_Builder, pins[4]);
    }

    void
    ForNode::compile_output_pin(Low::Util::StringBuilder &p_Builder,
                                NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_pin_checked(p_PinId);

      LOW_ASSERT(l_Pin->direction == PinDirection::Output,
                 "Pin is not an output pin");

      p_Builder.append("__for_index").append(id.Get());
    }

    void register_nodes()
    {
      {
        Low::Util::Name l_TypeName = N(FlodeSyntaxFunction);
        register_node(l_TypeName, &function_create_instance);

        register_spawn_node("Syntax", "Function", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeSyntaxGetVariable);
        register_node(l_TypeName, &getvariable_create_instance);

        register_spawn_node("Syntax", "Get variable", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeSyntaxSetVariable);
        register_node(l_TypeName, &setvariable_create_instance);

        register_spawn_node("Syntax", "Set variable", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeSyntaxReturnNumber);
        register_node(l_TypeName, &returnnumber_create_instance);

        register_spawn_node("Syntax", "Return number", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeSyntaxReturnBool);
        register_node(l_TypeName, &returnbool_create_instance);

        register_spawn_node("Syntax", "Return bool", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeSyntaxIf);
        register_node(l_TypeName, []() { return new IfNode; });

        register_spawn_node("Syntax", "If", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeSyntaxFor);
        register_node(l_TypeName, []() { return new ForNode; });

        register_spawn_node("Syntax", "For", l_TypeName);
      }
    }

  } // namespace SyntaxNodes
} // namespace Flode
