#pragma once
#include "FlodeApi.h"

#include <imgui_node_editor.h>

#include "LowEditorMetadata.h"

#include "LowUtilContainers.h"
#include "LowUtilVariant.h"
#include "LowUtilString.h"

#include "IconsCodicons.h"

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
    String,
    Handle,
    Bool,
    Vector2,
    Vector3,
    Quaternion,
    Enum,
    Dynamic
  };

  enum class PinStringType
  {
    String,
    Name
  };

  enum class PinDirection
  {
    Input,
    Output
  };

  struct Pin;

  void ShowToolTip(const char *p_Label, ImColor p_Color);

  void initialize();

  Low::Util::List<Low::Editor::TypeMetadata> &get_exposed_types();

  void setup_default_value_for_pin(Pin *p_Pin);

  Low::Util::String pin_direction_to_string(PinDirection p_Direction);
  Low::Util::String FLODE_API pin_type_to_string(PinType p_Type);

  PinType FLODE_API string_to_pin_type(Low::Util::String p_String);

  PinType property_type_to_pin_type(u8 p_PropertyType);

  PinStringType property_type_to_pin_string_type(u8 p_PropertyType);

  PinType variant_type_to_pin_type(u8 p_VariantType);

  struct FLODE_API Pin
  {
    NodeEd::PinId id;
    Low::Util::String title;
    PinType type;
    PinDirection direction;
    NodeEd::NodeId nodeId;

    u16 typeId;
    PinStringType stringType;

    Low::Util::Variant defaultValue;
    Low::Util::String defaultStringValue;
  };

  Low::Util::String get_pin_default_value_as_string(Pin *p_Pin);

  struct Graph;

  struct FLODE_API Node
  {
    ~Node();

    NodeEd::NodeId id;
    Low::Util::List<Pin *> pins;

    Low::Util::Name typeName;

    virtual void initialize()
    {
    }

    Graph *graph;

    Pin *create_pin(PinDirection p_Direction,
                    Low::Util::String p_Title, PinType p_Type,
                    u16 p_TypeId, u64 p_PinId = 0);
    Pin *create_pin(PinDirection p_Direction,
                    Low::Util::String p_Title, PinType p_Type,
                    u64 p_PinId = 0);
    Pin *create_handle_pin(PinDirection p_Direction,
                           Low::Util::String p_Title, u16 p_TypeId,
                           u64 p_PinId = 0);
    Pin *Node::create_enum_pin(PinDirection p_Direction,
                               Low::Util::String p_Title,
                               u16 p_EnumId, u64 p_PinId = 0);

    Pin *create_string_pin(PinDirection p_Direction,
                           Low::Util::String p_Title,
                           PinStringType p_StringType,
                           u64 p_PinId = 0);

    Pin *create_pin_from_property_info(
        PinDirection p_Direction, Low::Util::String p_Title,
        Low::Util::RTTI::PropertyInfo &, u64 p_PinId = 0);

    Pin *create_pin_from_rtti(PinDirection p_Direction,
                              Low::Util::String p_Title,
                              u32 p_PropertyType, u16 p_TypeId,
                              u64 p_PinId = 0);

    Pin *create_pin_from_variant(PinDirection p_Direction,
                                 Low::Util::String p_Title,
                                 Low::Util::Variant &p_Variant,
                                 u64 p_PinId = 0);

    Pin *find_pin(NodeEd::PinId p_PinId) const;
    Pin *find_pin_checked(NodeEd::PinId p_PinId) const;

    Pin *find_output_pin_checked(NodeEd::PinId p_PinId) const;

    virtual Low::Util::String get_name(NodeNameType p_Type) const;
    virtual Low::Util::String get_subtitle(NodeNameType p_Type) const;
    virtual Low::Util::String get_icon() const;

    virtual ImU32 get_color() const
    {
      return IM_COL32_BLACK;
    }

    virtual void setup_default_pins()
    {
    }

    virtual void on_pin_connected(Pin *p_Pin)
    {
    }

    virtual bool
    accept_dynamic_pin_connection(Pin *p_Pin,
                                  Pin *p_ConnectedPin) const
    {
      return true;
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

  struct FLODE_API Variable
  {
    Low::Util::String name;
    PinType type;
    u16 typeId;
  };

  struct FLODE_API Graph
  {
    Low::Util::Name m_Name;
    Low::Util::List<Low::Util::Name> m_Namespace;

    Low::Util::Handle m_ContextHandle;

    bool has_context_handle() const;
    u16 get_context_handle_type_id() const;

    bool m_Internal;

    Low::Util::List<Node *> m_Nodes;
    Low::Util::List<Link *> m_Links;

    Low::Util::List<Variable *> m_Variables;

    Variable *find_variable(Low::Util::String) const;

    u64 m_IdCounter = 1;

    Node *find_node(NodeEd::NodeId) const;

    bool can_create_link(NodeEd::PinId p_InputPin,
                         NodeEd::PinId p_OutputPin);

    Pin *find_pin(NodeEd::PinId p_PinId) const;

    Link *create_link(NodeEd::PinId p_InputPin,
                      NodeEd::PinId p_OutputPin);

    Link *create_link_castable(NodeEd::PinId p_InputPin,
                               NodeEd::PinId p_OutputPin);

    void delete_link(NodeEd::LinkId p_LinkId);
    void delete_node(NodeEd::NodeId p_NodeId);

    void serialize_node(const Node *p_Node,
                        Low::Util::Yaml::Node &p_Yaml,
                        bool p_StoreNodePositions = true) const;

    u64 deserialize_node(Low::Util::Yaml::Node &p_Yaml, Node **p_Node,
                         u64 p_IdStartValue, bool p_UseIds = true);

    void serialize(Low::Util::Yaml::Node &p_Node,
                   bool p_StoreNodePositions = true) const;
    void deserialize(Low::Util::Yaml::Node &p_Node);

    Node *create_node(Low::Util::Name p_TypeName,
                      bool p_SetupPins = true);

    void disconnect_pin(NodeEd::PinId p_PinId);
    bool is_pin_connected(NodeEd::PinId p_PinId) const;
    NodeEd::PinId get_connected_pin(NodeEd::PinId p_PinId) const;

    void clean_unconnected_links();

    void compile() const;
    void continue_compilation(Low::Util::StringBuilder &p_Builder,
                              Pin *p_Pin) const;
  };

  FLODE_API void register_nodes_for_type(u16 p_TypeId);

  FLODE_API void register_node(Low::Util::Name p_TypeName,
                               std::function<Node *()>);

  FLODE_API void register_spawn_node(Low::Util::String p_Category,
                                     Low::Util::String p_Title,
                                     Low::Util::Name p_TypeName);

  FLODE_API void register_cast_node(PinType p_FromType,
                                    PinType p_ToType,
                                    Low::Util::Name p_TypeName);

  Low::Util::Map<Low::Util::String,
                 Low::Util::Map<Low::Util::String, Low::Util::Name>> &
  get_node_types();

  Node *spawn_node_of_type(Low::Util::Name p_TypeName);

  bool can_cast(PinType p_FromType, PinType p_ToType);
  Low::Util::Name get_cast_node_typename(PinType p_FromType,
                                         PinType p_ToType);
} // namespace Flode
