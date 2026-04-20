#pragma once

#include "LowEditorApi.h"
#include "LowEditorNodeGraph.h"

#include "LowUtilContainers.h"
#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilString.h"
#include "LowUtilVariant.h"

#include <functional>

namespace Low {
  namespace Editor {
    namespace VisualScripting {
      struct Graph;

      enum class PinType
      {
        Execution,
        Bool,
        Number,
        Vector2,
        Vector3,
        Vector4,
        Quaternion,
        String,
        Handle,
        Dynamic
      };

      enum class StringSubtype
      {
        String,
        Name
      };

      enum class NumberSubtype
      {
        Float,
        Int32,
        UInt32,
        UInt64
      };

      enum class PinContainerType
      {
        None,
        List
      };

      enum class PinWidget
      {
        None,
        DefaultValue
      };

      struct LOW_EDITOR_API Pin
      {
        PinId pin;
        Util::String display_name;
        PinType type = PinType::Dynamic;
        StringSubtype string_subtype = StringSubtype::String;
        NumberSubtype number_subtype = NumberSubtype::Float;
        Util::TypeIdentifier handle_type;
        PinContainerType container_type = PinContainerType::None;
        Util::Variant default_value;
        PinWidget widget = PinWidget::DefaultValue;
        bool show_default_value_when_unlinked = true;

        bool is_valid() const
        {
          return pin.is_valid();
        }
      };

      struct LOW_EDITOR_API Node
      {
        NodeId node;
        Util::Name node_class;
        Util::String title;
        Util::String subtitle;
        Util::String category;

        bool is_valid() const
        {
          return node.is_valid();
        }
      };

      struct LOW_EDITOR_API NodeClass
      {
        virtual ~NodeClass() = default;

        virtual Util::Name get_name() const = 0;

        virtual Util::String get_title(const Graph &p_Graph,
                                       NodeId p_NodeId) const;

        virtual Util::String get_subtitle(const Graph &p_Graph,
                                          NodeId p_NodeId) const;

        virtual Util::String get_category(const Graph &p_Graph,
                                          NodeId p_NodeId) const;

        virtual Util::String get_icon(const Graph &p_Graph,
                                      NodeId p_NodeId) const;

        virtual ImU32 get_color(const Graph &p_Graph,
                                NodeId p_NodeId) const;

        virtual void
        setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                           const NodeGraphSchema *p_Schema) const
        {
          (void)p_Graph;
          (void)p_NodeId;
          (void)p_Schema;
        }
      };

      struct LOW_EDITOR_API Graph
      {
        NodeGraph graph;
        Util::Map<NodeId, Node> node_metadata;
        Util::Map<PinId, Pin> pin_metadata;
        Util::Map<Util::Name, NodeClass *> node_classes;
        u64 id_counter = 1;

        NodeGraphMutationResult<Editor::Node>
        add_node(const Editor::Node &p_Node, const Node &p_Metadata,
                 const NodeGraphSchema *p_Schema = nullptr);

        NodeGraphMutationResult<Editor::Pin>
        add_pin(const Editor::Pin &p_Pin, const Pin &p_Metadata,
                const NodeGraphSchema *p_Schema = nullptr);

        NodeGraphMutationResult<Editor::Link>
        add_link(const Editor::Link &p_Link,
                 const NodeGraphSchema *p_Schema = nullptr);

        bool remove_node(NodeId p_NodeId);
        bool remove_pin(PinId p_PinId);
        bool remove_link(LinkId p_LinkId);

        Node *find_node(NodeId p_NodeId);
        const Node *find_node(NodeId p_NodeId) const;

        Pin *find_pin(PinId p_PinId);
        const Pin *find_pin(PinId p_PinId) const;

        void register_node_class(NodeClass &p_NodeClass);
        NodeClass *find_node_class(Util::Name p_NodeClass);
        const NodeClass *
        find_node_class(Util::Name p_NodeClass) const;

        NodeGraphMutationResult<Editor::Node>
        create_node(Util::Name p_NodeClass,
                    const Math::Vector2 &p_Position,
                    const NodeGraphSchema *p_Schema = nullptr);

        NodeId allocate_node_id();
        PinId allocate_pin_id();
      };

      struct LOW_EDITOR_API Schema : public NodeGraphSchema
      {
        Graph *graph = nullptr;

        Schema() = default;
        Schema(Graph &p_Graph) : graph(&p_Graph)
        {
        }

        void set_graph(Graph &p_Graph)
        {
          graph = &p_Graph;
        }

        virtual NodeGraphValidationResult
        can_create_pin(const NodeGraph &p_Graph,
                       const Editor::Pin &p_Pin) const override;

        virtual NodeGraphValidationResult
        validate_link(const NodeGraph &p_Graph,
                      const Editor::Pin &p_StartPin,
                      const Editor::Pin &p_EndPin) const override;

        virtual bool allows_multiple_links_per_pin(
            const NodeGraph &p_Graph,
            const Editor::Pin &p_Pin) const override;

      private:
        const Graph *get_graph(const NodeGraph &p_Graph) const;
        const Pin *get_pin_metadata(const Editor::Pin &p_Pin) const;
      };

      struct LOW_EDITOR_API NodeRenderer
          : public NodeGraphNodeRenderer
      {
        Graph *graph = nullptr;
        Math::Vector2 default_node_size =
            Math::Vector2(260.0f, 140.0f);
        float title_height = 30.0f;
        float border_rounding = 3.0f;
        float pin_radius = 6.0f;

        NodeRenderer() = default;
        NodeRenderer(Graph &p_Graph) : graph(&p_Graph)
        {
        }

        void set_graph(Graph &p_Graph)
        {
          graph = &p_Graph;
        }

        virtual Math::Vector2
        get_node_size(const NodeGraphEditorContext &p_Context,
                      const Editor::Node &p_Node) const override;

        virtual void render_node(NodeGraphEditorContext &p_Context,
                                 Editor::Node &p_Node,
                                 const ImVec2 &p_ScreenMin,
                                 const ImVec2 &p_ScreenMax) override;

        virtual bool get_pin_anchor(
            const NodeGraphEditorContext &p_Context,
            const Editor::Node &p_Node, const Editor::Pin &p_Pin,
            const ImVec2 &p_ScreenMin, const ImVec2 &p_ScreenMax,
            ImVec2 &p_Anchor) const override;
      };

      struct LOW_EDITOR_API GraphRenderer
          : public NodeGraphEditorRenderer
      {
        Graph *graph = nullptr;
        NodeRenderer node_renderer;

        GraphRenderer() = default;
        GraphRenderer(Graph &p_Graph)
            : graph(&p_Graph), node_renderer(p_Graph)
        {
        }

        void set_graph(Graph &p_Graph)
        {
          graph = &p_Graph;
          node_renderer.set_graph(p_Graph);
        }

        virtual NodeGraphNodeRenderer *
        get_node_renderer(NodeGraphEditorContext &p_Context,
                          Editor::Node &p_Node) override;
      };

      LOW_EDITOR_API const char *pin_type_to_string(PinType p_Type);

      LOW_EDITOR_API PinType
      variant_type_to_pin_type(u8 p_VariantType);

      LOW_EDITOR_API u8 pin_to_variant_type(const Pin &p_Pin);

      LOW_EDITOR_API Util::Variant
      default_value_for_pin(const Pin &p_Pin);

      LOW_EDITOR_API Editor::Pin make_input_pin(Graph &p_Graph,
                                                NodeId p_NodeId);

      LOW_EDITOR_API Editor::Pin make_output_pin(Graph &p_Graph,
                                                 NodeId p_NodeId);

      LOW_EDITOR_API Pin
      make_execution_pin_metadata(Util::String p_DisplayName);

      LOW_EDITOR_API Pin make_bool_pin_metadata(
          Util::String p_DisplayName, bool p_DefaultValue = false);

      LOW_EDITOR_API Pin make_number_pin_metadata(
          Util::String p_DisplayName,
          NumberSubtype p_NumberSubtype = NumberSubtype::Float,
          PinContainerType p_ContainerType = PinContainerType::None);

      LOW_EDITOR_API Pin make_string_pin_metadata(
          Util::String p_DisplayName,
          StringSubtype p_StringSubtype = StringSubtype::String,
          PinContainerType p_ContainerType = PinContainerType::None);

      LOW_EDITOR_API Pin make_handle_pin_metadata(
          Util::String p_DisplayName,
          Util::TypeIdentifier p_HandleType = Util::TypeIdentifier(),
          PinContainerType p_ContainerType = PinContainerType::None);

      LOW_EDITOR_API Pin make_vector2_pin_metadata(
          Util::String p_DisplayName,
          PinContainerType p_ContainerType = PinContainerType::None);

      LOW_EDITOR_API Pin make_vector3_pin_metadata(
          Util::String p_DisplayName,
          PinContainerType p_ContainerType = PinContainerType::None);

      LOW_EDITOR_API Pin make_dynamic_pin_metadata(
          Util::String p_DisplayName,
          PinContainerType p_ContainerType = PinContainerType::None);
    } // namespace VisualScripting
  } // namespace Editor
} // namespace Low
