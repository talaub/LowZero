#include "FlodeHandleNodes.h"

#include "LowEditorMainWindow.h"
#include "LowEditorMetadata.h"

namespace Flode {
  namespace HandleNodes {
    ImU32 g_HandleColor = IM_COL32(76, 131, 195, 255);

    FindByNameNode::FindByNameNode(u16 p_TypeId)
    {
      m_TypeInfo = Low::Util::Handle::get_type_info(p_TypeId);

      m_CachedName = Low::Util::String("Find ") +
                     m_TypeInfo.name.c_str() + " by name";
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

    void FindByNameNode::setup_default_pins()
    {
      create_string_pin(PinDirection::Input, "", PinStringType::Name);
      create_handle_pin(PinDirection::Output, "", m_TypeInfo.typeId);
    }

    void FindByNameNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      Pin *l_InputPin = pins[0];

      Low::Editor::TypeMetadata &l_TypeMetadata =
          Low::Editor::get_type_metadata(m_TypeInfo.typeId);

      p_Builder.append(l_TypeMetadata.fullTypeString);
      p_Builder.append("::find_by_name(");
      compile_input_pin(p_Builder, l_InputPin->id);
      p_Builder.append(")");
    }

    GetNode::GetNode(u16 p_TypeId, Low::Util::Name p_PropertyName)
    {
      m_TypeInfo = Low::Util::Handle::get_type_info(p_TypeId);
      m_PropertyInfo = m_TypeInfo.properties[p_PropertyName];

      m_CachedName = Low::Util::String("Get ") +
                     m_PropertyInfo.name.c_str() + " of " +
                     m_TypeInfo.name.c_str();
    }

    ImU32 GetNode::get_color() const
    {
      return g_HandleColor;
    }

    Low::Util::String GetNode::get_name(NodeNameType p_Type) const
    {
      return m_CachedName;
    }

    void GetNode::setup_default_pins()
    {
      create_handle_pin(PinDirection::Input, "", m_TypeInfo.typeId);
      create_pin_from_property_info(PinDirection::Output, "",
                                    m_PropertyInfo);
    }

    void
    GetNode::compile_output_pin(Low::Util::StringBuilder &p_Builder,
                                NodeEd::PinId p_PinId) const
    {
      Pin *l_InputPin = pins[0];

      Low::Editor::TypeMetadata &l_TypeMetadata =
          Low::Editor::get_type_metadata(m_TypeInfo.typeId);

      for (auto it = l_TypeMetadata.properties.begin();
           it != l_TypeMetadata.properties.end(); ++it) {
        if (it->name == m_PropertyInfo.name) {
          compile_input_pin(p_Builder, l_InputPin->id);
          p_Builder.append(".");
          p_Builder.append(it->getterName);
          p_Builder.append("(");
          p_Builder.append(")");
          break;
        }
      }
    }

    SetNode::SetNode(u16 p_TypeId, Low::Util::Name p_PropertyName)
    {
      m_TypeInfo = Low::Util::Handle::get_type_info(p_TypeId);
      m_PropertyInfo = m_TypeInfo.properties[p_PropertyName];

      m_CachedName = Low::Util::String("Set ") +
                     m_PropertyInfo.name.c_str() + " of " +
                     m_TypeInfo.name.c_str();
    }

    ImU32 SetNode::get_color() const
    {
      return g_HandleColor;
    }

    Low::Util::String SetNode::get_name(NodeNameType p_Type) const
    {
      return m_CachedName;
    }

    void SetNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Flow);
      create_pin(PinDirection::Output, "", PinType::Flow);

      create_handle_pin(PinDirection::Input, m_TypeInfo.name.c_str(),
                        m_TypeInfo.typeId);
      create_pin_from_property_info(PinDirection::Input, "",
                                    m_PropertyInfo);
    }

    void SetNode::compile(Low::Util::StringBuilder &p_Builder) const
    {
      Pin *l_InputPin = pins[0];

      Low::Editor::TypeMetadata &l_TypeMetadata =
          Low::Editor::get_type_metadata(m_TypeInfo.typeId);

      for (auto it = l_TypeMetadata.properties.begin();
           it != l_TypeMetadata.properties.end(); ++it) {
        if (it->name == m_PropertyInfo.name) {
          compile_input_pin(p_Builder, l_InputPin->id);
          p_Builder.append(".");
          p_Builder.append(it->setterName);
          p_Builder.append("(");
          compile_input_pin(p_Builder, pins[3]->id);
          p_Builder.append(");").endl();
          break;
        }
      }

      graph->continue_compilation(p_Builder, pins[1]);
      // compile_input_pin(p_Builder, NodeEd::PinId p_PinId)
    }
  } // namespace HandleNodes
} // namespace Flode
