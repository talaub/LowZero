#include "FlodeSyntaxNodes.h"
#include <cstring>

#include "IconsFontAwesome5.h"

namespace Flode {
  namespace SyntaxNodes {

    ImU32 g_SyntaxColor = IM_COL32(138, 46, 119, 255);

    Low::Util::String
    FunctionNode::get_name(NodeNameType p_Type) const
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

        Pin *i_Pin =
            create_pin(PinDirection::Output, i_Param.name.c_str(),
                       i_Param.type, i_Param.pinId.Get());

        i_Param.pinId = i_Pin->id;
      }

      graph->clean_unconnected_links();
    }

    void FunctionNode::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      for (auto it = m_Parameters.begin(); it != m_Parameters.end();
           ++it) {
        Low::Util::Yaml::Node i_ParamNode;

        i_ParamNode["name"] = it->name.c_str();
        i_ParamNode["type"] = pin_type_to_string(it->type).c_str();
        i_ParamNode["pin_id"] = it->pinId.Get();

        p_Node["parameters"].push_back(i_ParamNode);
      }
    }

    void FunctionNode::deserialize(Low::Util::Yaml::Node &p_Node)
    {
      if (p_Node["parameters"]) {
        for (auto it = p_Node["parameters"].begin();
             it != p_Node["parameters"].end(); ++it) {
          Low::Util::Yaml::Node &i_Node = *it;

          FunctionNodeParameter i_Param;
          i_Param.name = LOW_YAML_AS_NAME(i_Node["name"]);
          i_Param.type =
              string_to_pin_type(LOW_YAML_AS_STRING(i_Node["type"]));
          i_Param.pinId = i_Node["pin_id"].as<u64>();

          m_Parameters.push_back(i_Param);
        }
      }
    }

    void FunctionNode::render_data()
    {
      ImGui::Text("Parameters");

      bool l_Changed = false;

      if (ImGui::Button(ICON_FA_PLUS "")) {
        FunctionNodeParameter l_Param;
        l_Param.name = "new";
        l_Param.type = PinType::Number;
        l_Param.pinId = 0;

        m_Parameters.push_back(l_Param);

        l_Changed = true;
      }

      int l_ParamCount = 0;

      Low::Util::List<PinType> l_Types = {PinType::String,
                                          PinType::Number};

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

        if (l_Result) {
          l_Changed = true;

          // i_Parameter.type = static_cast<PinType>(l_CurrentValue);
          i_Parameter.type = l_Types[l_CurrentValue];
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_TRASH "")) {
          it = m_Parameters.erase(it);
          l_Changed = true;
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
        break;
      case PinType::Number:
        return "float";
        break;
      }

      _LOW_ASSERT(false);
    }

    void
    FunctionNode::compile(Low::Util::StringBuilder &p_Builder) const
    {
      p_Builder.append("void ");
      p_Builder.append("test_function");
      p_Builder.append("(");

      int l_ParameterCount = 0;
      for (auto it = m_Parameters.begin(); it != m_Parameters.end();
           ++it) {
        if (l_ParameterCount) {
          p_Builder.append(", ");
        }

        p_Builder.append(pin_type_to_cpp_string(it->type));
        p_Builder.append(" ");

        p_Builder.append("p_");
        p_Builder.append(it->name);

        l_ParameterCount++;
      }

      p_Builder.append(") {\n");

      NodeEd::PinId l_FlowOutputPinId = pins[0]->id;
      if (graph->is_pin_connected(l_FlowOutputPinId)) {
        NodeEd::PinId l_ConnectedFlowPinId =
            graph->get_connected_pin(l_FlowOutputPinId);

        Pin *l_ConnectedFlowPin =
            graph->find_pin(l_ConnectedFlowPinId);
        Node *l_ConnectedNode =
            graph->find_node(l_ConnectedFlowPin->nodeId);

        l_ConnectedNode->compile(p_Builder);
      }

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
      return new FunctionNode;
    }

    void register_nodes()
    {
      {
        Low::Util::Name l_TypeName = N(FlodeSyntaxFunction);
        register_node(l_TypeName, &function_create_instance);

        register_spawn_node("Syntax", "Function", l_TypeName);
      }
    }

  } // namespace SyntaxNodes
} // namespace Flode
