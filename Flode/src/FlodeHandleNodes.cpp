#include "FlodeHandleNodes.h"

#include "LowEditor.h"
#include "LowEditorMainWindow.h"
#include "LowEditorMetadata.h"

namespace Flode {
  namespace HandleNodes {
    ImU32 g_HandleColor = IM_COL32(76, 131, 195, 255);

    TypeIdNode::TypeIdNode(u16 p_TypeId)
    {
      m_TypeMetadata = Low::Editor::get_type_metadata(p_TypeId);

      m_CachedName = m_TypeMetadata.friendlyName + " type id";
    }

    ImU32 TypeIdNode::get_color() const
    {
      return g_HandleColor;
    }

    Low::Util::String TypeIdNode::get_name(NodeNameType p_Type) const
    {
      return m_CachedName;
    }

    void TypeIdNode::setup_default_pins()
    {
      create_pin(PinDirection::Output, "", PinType::Number);
    }

    void TypeIdNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      p_Builder.append(m_TypeMetadata.fullTypeString);
      p_Builder.append("::TYPE_ID");
    }

    InstanceCountNode::InstanceCountNode(u16 p_TypeId)
    {
      m_TypeMetadata = Low::Editor::get_type_metadata(p_TypeId);

      m_CachedName = m_TypeMetadata.friendlyName + " instance count";
    }

    ImU32 InstanceCountNode::get_color() const
    {
      return g_HandleColor;
    }

    Low::Util::String
    InstanceCountNode::get_name(NodeNameType p_Type) const
    {
      return m_CachedName;
    }

    void InstanceCountNode::setup_default_pins()
    {
      create_pin(PinDirection::Output, "", PinType::Number);
    }

    void InstanceCountNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      p_Builder.append(m_TypeMetadata.fullTypeString);
      p_Builder.append("::living_count()");
    }

    GetInstanceByIndexNode::GetInstanceByIndexNode(u16 p_TypeId)
    {
      m_TypeMetadata = Low::Editor::get_type_metadata(p_TypeId);

      m_CachedName = Low::Util::String("Get instance by index");
    }

    ImU32 GetInstanceByIndexNode::get_color() const
    {
      return g_HandleColor;
    }

    Low::Util::String
    GetInstanceByIndexNode::get_name(NodeNameType p_Type) const
    {
      return m_CachedName;
    }

    Low::Util::String
    GetInstanceByIndexNode::get_subtitle(NodeNameType p_Type) const
    {
      return m_TypeMetadata.friendlyName;
    }

    void GetInstanceByIndexNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "Index", PinType::Number);
      create_handle_pin(PinDirection::Output, "",
                        m_TypeMetadata.typeId);
    }

    void GetInstanceByIndexNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      Pin *l_InputPin = pins[0];

      p_Builder.append(m_TypeMetadata.fullTypeString);
      p_Builder.append("::living_instances()[");
      compile_input_pin(p_Builder, l_InputPin->id);
      p_Builder.append("]");
    }

    FindByNameNode::FindByNameNode(u16 p_TypeId)
    {
      m_TypeMetadata = Low::Editor::get_type_metadata(p_TypeId);

      m_CachedName = Low::Util::String("Find by name");
    }

    ImU32 FindByNameNode::get_color() const
    {
      return g_HandleColor;
    }

    Low::Util::String
    FindByNameNode::get_name(NodeNameType p_Type) const
    {
      return m_CachedName;
    }

    Low::Util::String
    FindByNameNode::get_subtitle(NodeNameType p_Type) const
    {
      return m_TypeMetadata.friendlyName;
    }

    void FindByNameNode::setup_default_pins()
    {
      create_string_pin(PinDirection::Input, "Name",
                        PinStringType::Name);
      create_handle_pin(PinDirection::Output, "",
                        m_TypeMetadata.typeId);
    }

    void FindByNameNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      Pin *l_InputPin = pins[0];

      p_Builder.append(m_TypeMetadata.fullTypeString);
      p_Builder.append("::find_by_name(");
      compile_input_pin(p_Builder, l_InputPin->id);
      p_Builder.append(")");
    }

    GetNode::GetNode(u16 p_TypeId, Low::Util::Name p_PropertyName)
    {
      m_TypeMetadata = Low::Editor::get_type_metadata(p_TypeId);
      m_PropertyMetadata =
          m_TypeMetadata.find_property_by_name(p_PropertyName);

      m_CachedName =
          Low::Util::String("Get ") + m_PropertyMetadata.friendlyName;
    }

    ImU32 GetNode::get_color() const
    {
      return g_HandleColor;
    }

    Low::Util::String GetNode::get_name(NodeNameType p_Type) const
    {
      return m_CachedName;
    }

    Low::Util::String GetNode::get_subtitle(NodeNameType p_Type) const
    {
      return m_TypeMetadata.friendlyName;
    }

    void GetNode::setup_default_pins()
    {
      create_handle_pin(PinDirection::Input,
                        m_TypeMetadata.friendlyName,
                        m_TypeMetadata.typeId);
      create_pin_from_property_info(PinDirection::Output, "",
                                    m_PropertyMetadata.propInfo);
    }

    void
    GetNode::compile_output_pin(Low::Util::StringBuilder &p_Builder,
                                NodeEd::PinId p_PinId) const
    {
      Pin *l_InputPin = pins[0];

      compile_input_pin(p_Builder, l_InputPin->id);
      p_Builder.append(".");
      p_Builder.append(m_PropertyMetadata.getterName);
      p_Builder.append("(");
      p_Builder.append(")");
    }

    SetNode::SetNode(u16 p_TypeId, Low::Util::Name p_PropertyName)
    {
      m_TypeMetadata = Low::Editor::get_type_metadata(p_TypeId);
      m_PropertyMetadata =
          m_TypeMetadata.find_property_by_name(p_PropertyName);

      m_CachedName =
          Low::Util::String("Set ") + m_PropertyMetadata.friendlyName;
    }

    ImU32 SetNode::get_color() const
    {
      return g_HandleColor;
    }

    Low::Util::String SetNode::get_name(NodeNameType p_Type) const
    {
      return m_CachedName;
    }

    Low::Util::String SetNode::get_subtitle(NodeNameType p_Type) const
    {
      return m_TypeMetadata.friendlyName;
    }

    void SetNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Flow);
      create_pin(PinDirection::Output, "", PinType::Flow);

      create_handle_pin(PinDirection::Input,
                        m_TypeMetadata.friendlyName,
                        m_TypeMetadata.typeId);
      create_pin_from_property_info(PinDirection::Input,
                                    m_PropertyMetadata.friendlyName,
                                    m_PropertyMetadata.propInfo);
    }

    void SetNode::compile(Low::Util::StringBuilder &p_Builder) const
    {
      Pin *l_InputPin = pins[0];

      compile_input_pin(p_Builder, l_InputPin->id);
      p_Builder.append(".");
      p_Builder.append(m_PropertyMetadata.setterName);
      p_Builder.append("(");
      compile_input_pin(p_Builder, pins[3]->id);
      p_Builder.append(");").endl();

      graph->continue_compilation(p_Builder, pins[1]);
    }

    FunctionNode::FunctionNode(u16 p_TypeId,
                               Low::Util::Name p_FunctionName)
    {
      m_TypeMetadata = Low::Editor::get_type_metadata(p_TypeId);
      bool l_Found = false;
      for (auto it = m_TypeMetadata.functions.begin();
           it != m_TypeMetadata.functions.end(); ++it) {
        if (it->name == p_FunctionName) {
          l_Found = true;
          m_FunctionMetadata = *it;
        }
      }

      LOW_ASSERT(l_Found, "Could not find function of name in type");

      m_CachedName = m_FunctionMetadata.friendlyName;
    }

    ImU32 FunctionNode::get_color() const
    {
      return g_HandleColor;
    }

    Low::Util::String
    FunctionNode::get_name(NodeNameType p_Type) const
    {
      return m_CachedName;
    }

    Low::Util::String
    FunctionNode::get_subtitle(NodeNameType p_Type) const
    {
      return m_TypeMetadata.friendlyName;
    }

    void FunctionNode::setup_default_pins()
    {
      if (m_FunctionMetadata.hasReturnValue) {
        create_pin_from_rtti(
            PinDirection::Output, "",
            m_FunctionMetadata.functionInfo.type,
            m_FunctionMetadata.functionInfo.handleType);
      } else {
        create_pin(PinDirection::Input, "", PinType::Flow);
        create_pin(PinDirection::Output, "", PinType::Flow);
      }

      // If the function is an instance member (not static) we need to
      // receive the instance to call it on using an additional input
      // pin
      if (!m_FunctionMetadata.isStatic) {
        create_handle_pin(PinDirection::Input,
                          m_TypeMetadata.name.c_str(),
                          m_TypeMetadata.typeId);
      }

      for (auto it = m_FunctionMetadata.parameters.begin();
           it != m_FunctionMetadata.parameters.end(); ++it) {
        create_pin_from_rtti(PinDirection::Input, it->friendlyName,
                             it->paramInfo.type,
                             it->paramInfo.handleType);
      }
    }

    void
    FunctionNode::compile(Low::Util::StringBuilder &p_Builder) const
    {
      _LOW_ASSERT(!m_FunctionMetadata.hasReturnValue);

      u32 l_PinOffset = 2;

      if (m_FunctionMetadata.isStatic) {
        p_Builder.append(m_TypeMetadata.fullTypeString);
      } else {
        compile_input_pin(p_Builder, pins[l_PinOffset]->id);

        l_PinOffset += 1;
      }

      if (m_FunctionMetadata.isStatic) {
        p_Builder.append("::");
      } else {
        p_Builder.append(".");
      }

      p_Builder.append(m_FunctionMetadata.name).append("(");

      for (u32 i = 0; i < m_FunctionMetadata.parameters.size(); ++i) {
        if (i) {
          p_Builder.append(", ");
        }
        compile_input_pin(p_Builder, pins[i + l_PinOffset]->id);
      }

      p_Builder.append(");").endl();

      graph->continue_compilation(p_Builder, pins[1]);
    }

    void FunctionNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      _LOW_ASSERT(m_FunctionMetadata.hasReturnValue);

      u32 l_PinOffset = 1;

      if (m_FunctionMetadata.isStatic) {
        p_Builder.append(m_TypeMetadata.fullTypeString);
      } else {
        compile_input_pin(p_Builder, pins[l_PinOffset]->id);

        l_PinOffset += 1;
      }

      if (m_FunctionMetadata.isStatic) {
        p_Builder.append("::");
      } else {
        p_Builder.append(".");
      }

      p_Builder.append(m_FunctionMetadata.name).append("(");

      for (u32 i = 0; i < m_FunctionMetadata.parameters.size(); ++i) {
        if (i) {
          p_Builder.append(", ");
        }
        compile_input_pin(p_Builder, pins[i + l_PinOffset]->id);
      }

      p_Builder.append(")");
    }

    ForEachInstanceNode::ForEachInstanceNode(u16 p_TypeId)
    {
      m_TypeMetadata = Low::Editor::get_type_metadata(p_TypeId);

      m_CachedName = Low::Util::String("For each instance");
    }

    ImU32 ForEachInstanceNode::get_color() const
    {
      return g_HandleColor;
    }

    Low::Util::String
    ForEachInstanceNode::get_name(NodeNameType p_Type) const
    {
      return m_CachedName;
    }

    Low::Util::String
    ForEachInstanceNode::get_subtitle(NodeNameType p_Type) const
    {
      return m_TypeMetadata.friendlyName;
    }

    void ForEachInstanceNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Flow);
      create_pin(PinDirection::Output, "", PinType::Flow);
      create_pin(PinDirection::Output, "Loop", PinType::Flow);
      create_handle_pin(PinDirection::Output, "Instance",
                        m_TypeMetadata.typeId);
    }

    void ForEachInstanceNode::compile(
        Low::Util::StringBuilder &p_Builder) const
    {
      Low::Util::StringBuilder l_IndexVariableName;
      l_IndexVariableName.append("__foreach_index").append(id.Get());

      Low::Util::StringBuilder l_InstanceVariableName;
      l_InstanceVariableName.append("__foreach_instance")
          .append(id.Get());

      p_Builder.append("for (int ")
          .append(l_IndexVariableName.get())
          .append(" = 0; ")
          .append(l_IndexVariableName.get())
          .append(" < ")
          .append(m_TypeMetadata.fullTypeString)
          .append("::living_count(); ++")
          .append(l_IndexVariableName.get());
      p_Builder.append(") {").endl();
      p_Builder.append(m_TypeMetadata.fullTypeString)
          .append(" ")
          .append(l_InstanceVariableName.get())
          .append(" = ")
          .append(m_TypeMetadata.fullTypeString)
          .append("::living_instances()[")
          .append(l_IndexVariableName.get())
          .append("];")
          .endl();
      graph->continue_compilation(p_Builder, pins[2]);
      p_Builder.append("}").endl();

      graph->continue_compilation(p_Builder, pins[1]);
    }

    void ForEachInstanceNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_pin_checked(p_PinId);

      LOW_ASSERT(l_Pin->direction == PinDirection::Output,
                 "Pin is not an output pin");

      p_Builder.append("__foreach_instance").append(id.Get());
    }
  } // namespace HandleNodes
} // namespace Flode
