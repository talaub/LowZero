#pragma once
#include "FlodeApi.h"

#include <imgui_node_editor.h>

#include "LowUtilContainers.h"
#include "LowUtilVariant.h"
#include "LowUtilString.h"

namespace NodeEd = ax::NodeEditor;

namespace Flode {
  enum class NodeNameType
  {
    Full,
    Compact
  };

  enum class PinType
  {
    Flow,
    Number,
    String
  };

  enum class PinDirection
  {
    Input,
    Output
  };

  void setup_variant_for_pin_type(PinType p_PinType,
                                  Low::Util::Variant &p_Variant);

  Low::Util::String pin_direction_to_string(PinDirection p_Direction);
  Low::Util::String pin_type_to_string(PinType p_Type);

  PinType string_to_pin_type(Low::Util::String p_String);

  struct FLODE_API Pin
  {
    NodeEd::PinId id;
    Low::Util::String title;
    PinType type;
    PinDirection direction;
    NodeEd::NodeId nodeId;

    Low::Util::Variant defaultValue;
  };

  Low::Util::String get_pin_default_value_as_string(Pin *p_Pin);

  struct Graph;

  struct FLODE_API Node
  {
    NodeEd::NodeId id;
    Low::Util::List<Pin *> pins;

    Low::Util::Name typeName;

    Graph *graph;

    Pin *create_pin(PinDirection p_Direction,
                    Low::Util::String p_Title, PinType p_Type,
                    u64 p_PinId = 0);

    Pin *find_pin(NodeEd::PinId p_PinId) const;
    Pin *find_pin_checked(NodeEd::PinId p_PinId) const;

    Pin *find_output_pin_checked(NodeEd::PinId p_PinId) const;

    virtual Low::Util::String get_name(NodeNameType p_Type) const;

    virtual ImU32 get_color() const
    {
      return IM_COL32_BLACK;
    }
    virtual void setup_default_pins()
    {
    }

    virtual void render();

    virtual bool is_compact() const
    {
      return false;
    }

    virtual void serialize(Low::Util::Yaml::Node &p_Node) const
    {
    }

    virtual void deserialize(Low::Util::Yaml::Node &p_Node)
    {
    }

    virtual void render_data()
    {
    }

    virtual void compile(Low::Util::StringBuilder &p_Builder) const
    {
    }

    virtual void
    compile_output_pin(Low::Util::StringBuilder &p_Builder,
                       NodeEd::PinId p_PinId) const
    {
    }

    virtual void
    compile_input_pin(Low::Util::StringBuilder &p_Builder,
                      NodeEd::PinId p_PinId) const;

  protected:
    void render_header();
    void render_header_cosmetics();

    void render_input_pins();
    void render_output_pins();

    void render_pin(Flode::Pin *p_Pin);

    void default_render();
    void default_render_compact();

    ImVec2 m_HeaderMin;
    ImVec2 m_HeaderMax;
  };

  struct FLODE_API Link
  {
    NodeEd::LinkId id;
    NodeEd::PinId inputPinId;
    NodeEd::PinId outputPinId;

    Link()
    {
    }

    Link(NodeEd::LinkId p_Id, NodeEd::PinId p_InputPinId,
         NodeEd::PinId p_OutputPinId)
        : id(p_Id), inputPinId(p_InputPinId),
          outputPinId(p_OutputPinId)
    {
    }
  };

  struct FLODE_API Graph
  {
    Low::Util::List<Node *> m_Nodes;
    Low::Util::List<Link *> m_Links;

    u64 m_IdCounter = 1;

    Node *find_node(NodeEd::NodeId) const;

    bool can_create_link(NodeEd::PinId p_InputPin,
                         NodeEd::PinId p_OutputPin);

    Pin *find_pin(NodeEd::PinId p_PinId);

    Link *create_link(NodeEd::PinId p_InputPin,
                      NodeEd::PinId p_OutputPin);

    Link *create_link_castable(NodeEd::PinId p_InputPin,
                               NodeEd::PinId p_OutputPin);

    void delete_link(NodeEd::LinkId p_LinkId);

    void serialize(Low::Util::Yaml::Node &p_Node) const;
    void deserialize(Low::Util::Yaml::Node &p_Node);

    Node *create_node(Low::Util::Name p_TypeName);

    bool is_pin_connected(NodeEd::PinId p_PinId) const;
    NodeEd::PinId get_connected_pin(NodeEd::PinId p_PinId) const;

    void clean_unconnected_links();

    void compile() const;
  };

  typedef Node *(*create_node_callback)();

  void register_node(Low::Util::Name p_TypeName,
                     create_node_callback p_Callback);

  void register_spawn_node(Low::Util::String p_Category,
                           Low::Util::String p_Title,
                           Low::Util::Name p_TypeName);

  void register_cast_node(PinType p_FromType, PinType p_ToType,
                          Low::Util::Name p_TypeName);

  Low::Util::Map<Low::Util::String,
                 Low::Util::Map<Low::Util::String, Low::Util::Name>> &
  get_node_types();

  Node *spawn_node_of_type(Low::Util::Name p_TypeName);

  bool can_cast(PinType p_FromType, PinType p_ToType);
  Low::Util::Name get_cast_node_typename(PinType p_FromType,
                                         PinType p_ToType);
} // namespace Flode
