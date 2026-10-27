#pragma once

#include "LowEditorApi.h"
#include "LowEditorNodeGraph.h"
#include "LowEditorVisualScriptListDefault.h"

#include "LowUtilAssert.h"
#include "LowUtilContainers.h"
#include "LowUtilHandle.h"
#include "LowUtilHashing.h"
#include "LowUtilName.h"
#include "LowUtilSerialization.h"
#include "LowUtilString.h"
#include "LowUtilVariant.h"

#include <functional>
#include <memory>

namespace Low {
  namespace Editor {
    namespace VisualScript {
      struct Document;
      struct Graph;
      struct CompileProfileRegistry;
      struct CompileContext;

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
        Struct,
        Enum,
        Signal,
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

      struct LOW_EDITOR_API Pin : public Low::Editor::Pin
      {
        std::unique_ptr<Low::Editor::Pin> clone() const override
        {
          return std::make_unique<Pin>(*this);
        }
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
          return Low::Editor::Pin::is_valid();
        }
      };

      struct LOW_EDITOR_API Node : public Low::Editor::Node
      {
        virtual ~Node() = default;
        Util::Name spawn_entry;

        // Clone authored state through node_data; graph structure is
        // copied here.
        std::unique_ptr<Low::Editor::Node> clone() const final;

        virtual Util::Name get_name() const = 0;

        virtual Util::String get_title(const Graph &p_Graph) const;

        virtual Util::String get_subtitle(const Graph &p_Graph) const;

        virtual Util::String get_category(const Graph &p_Graph) const;

        virtual Util::String get_icon(const Graph &p_Graph) const;

        virtual ImU32 get_color(const Graph &p_Graph) const;

        virtual bool is_compact(const Graph &p_Graph) const
        {
          (void)p_Graph;
          return false;
        }

        virtual bool is_deletable(const Graph &p_Graph) const
        {
          (void)p_Graph;
          return true;
        }

        virtual Util::String
        get_compact_title(const Graph &p_Graph) const
        {
          return get_title(p_Graph);
        }

        virtual void
        setup_default_pins(Graph &p_Graph,
                           const NodeGraphSchema *p_Schema) const
        {
          (void)p_Graph;
          (void)p_Schema;
        }

        virtual float get_above_pins_height(const Graph &) const
        {
          return 0.0f;
        }

        virtual float get_below_pins_height(const Graph &) const
        {
          return 0.0f;
        }

        virtual void render_above_pins(Graph &,
                                       NodeGraphEditorContext &,
                                       const ImVec2 &,
                                       const ImVec2 &) const
        {
        }

        virtual void render_below_pins(Graph &,
                                       NodeGraphEditorContext &,
                                       const ImVec2 &,
                                       const ImVec2 &) const
        {
        }

        virtual void compile(Graph &p_Graph,
                             CompileContext &p_CompileContext) const
        {
        }

        virtual void
        compile_input_pin(Graph &p_Graph, PinId p_PinId,
                          CompileContext &p_CompileContext) const;

        virtual void
        compile_output_pin(Graph &p_Graph, PinId p_PinId,
                           CompileContext &p_CompileContext) const
        {
        }

        virtual bool
        can_connect_pin(Graph &p_Graph, PinId p_PinId,
                        const Pin &p_PinMetadata,
                        const Pin &p_OtherPinMetadata) const
        {
          (void)p_Graph;
          (void)p_PinId;
          (void)p_PinMetadata;
          (void)p_OtherPinMetadata;
          return true;
        }

        virtual void on_pin_connected(Graph &p_Graph, PinId p_PinId,
                                      PinId p_OtherPinId) const
        {
          (void)p_Graph;
          (void)p_PinId;
          (void)p_OtherPinId;
        }

        virtual void serialize(Util::Serial::Node &p_Data) const
        {
        }

        virtual void deserialize(const Util::Serial::Node &p_Data)
        {
        }

      private:
        friend struct Graph;
        std::unique_ptr<Node> (*m_Factory)() = nullptr;
      };

      struct LOW_EDITOR_API TypedNode : public Node
      {
        Util::TypeIdentifier handle_type;
        Util::Name member_name;
        void serialize(Util::Serial::Node &p_Data) const override;
        void deserialize(const Util::Serial::Node &p_Data) override;
      };

      struct LOW_EDITOR_API VariableNode : public Node
      {
        Util::String variable_name;
        void serialize(Util::Serial::Node &p_Data) const override;
        void deserialize(const Util::Serial::Node &p_Data) override;
      };

      struct NodeTypeDefinition
      {
        Util::Name name;
        std::unique_ptr<Node> (*create)() = nullptr;
      };

      struct LOW_EDITOR_API VariableUserData
      {
        virtual ~VariableUserData() = default;

        virtual void serialize(Util::Serial::Node &p_Node) const = 0;
        virtual void
        deserialize(const Util::Serial::Node &p_Node) = 0;
      };

      struct LOW_EDITOR_API Variable
      {
        Util::String name;
        PinType type = PinType::Dynamic;
        StringSubtype string_subtype = StringSubtype::String;
        NumberSubtype number_subtype = NumberSubtype::Float;
        Util::TypeIdentifier handle_type;
        PinContainerType container_type = PinContainerType::None;
        Util::Variant default_value;
        ListDefaultValue list_default_value;

        Util::SharedPtr<VariableUserData> user_data;

        template <typename T> T *get_user_data()
        {
          return dynamic_cast<T *>(user_data.get());
        }

        template <typename T> const T *get_user_data() const
        {
          return dynamic_cast<const T *>(user_data.get());
        }

        bool is_valid() const
        {
          return !name.empty() && type != PinType::Execution &&
                 type != PinType::Dynamic;
        }
      };

      // Asset-level scripting configuration. Editor extensions use
      // the same ID.
      using VariableUserDataFactory =
          std::function<Util::SharedPtr<VariableUserData>()>;

      struct LOW_EDITOR_API ScriptContextDefinition
      {
        Util::Name name;
        Util::Name default_compile_profile;
        std::function<void(Graph &)> register_node_libraries;
        VariableUserDataFactory create_variable_user_data;
      };

      struct LOW_EDITOR_API ScriptContextRegistry
      {
        Util::Map<Util::Name, ScriptContextDefinition> contexts;

        void
        register_context(const ScriptContextDefinition &p_Context);
        const ScriptContextDefinition *
        find_context(Util::Name p_ContextName) const;
      };

      LOW_EDITOR_API ScriptContextRegistry &
      get_script_context_registry();
      LOW_EDITOR_API CompileProfileRegistry &
      get_compile_profile_registry();
      LOW_EDITOR_API void
      register_common_node_libraries(Graph &p_Graph);

      struct LOW_EDITOR_API GameplaySystemVariableUserData
          : VariableUserData
      {
        bool gamemode_exposed = false;

        virtual void
        serialize(Util::Serial::Node &p_Node) const override;
        virtual void
        deserialize(const Util::Serial::Node &p_Node) override;
      };

      struct LOW_EDITOR_API NodeSpawnEntry
      {
        Util::Name id;
        Util::String category;
        Util::String title;
        Util::String subtitle;
        Util::String search_text;
        Util::Name node_type;
        std::function<void(Graph &, Node &)> initialize_node;

        bool is_valid() const
        {
          return id.is_valid() && node_type.is_valid();
        }
      };

      struct LOW_EDITOR_API CompileContext
      {
        Util::StringBuilder main_code;
        u32 indentation = 0;

        void append_indent();
        void append_line(const Util::String &p_Line);
        void begin_block(const Util::String &p_Line);
        void end_block(const Util::String &p_Line = "");
      };

      struct LOW_EDITOR_API CompileEntryPoint
      {
        NodeId node;
        PinId execution_output_pin;
        Util::String function_name;
        Util::String function_signature;

        bool is_valid() const
        {
          return node.is_valid() && execution_output_pin.is_valid() &&
                 !function_name.empty();
        }
      };

      struct LOW_EDITOR_API CompileProfileSettings
      {
        virtual ~CompileProfileSettings() = default;

        virtual Util::Name get_profile_name() const = 0;
        virtual void serialize(Util::Serial::Node &p_Node) const
        {
          (void)p_Node;
        }
        virtual void deserialize(const Util::Serial::Node &p_Node)
        {
          (void)p_Node;
        }
      };

      struct LOW_EDITOR_API DefaultCompileProfileSettings
          : public CompileProfileSettings
      {
        virtual Util::Name get_profile_name() const override
        {
          return N(vs_compile_default);
        }
      };

      struct LOW_EDITOR_API UiControllerCompileProfileSettings
          : public CompileProfileSettings
      {
        Util::String class_name = "VisualScriptUiController";

        virtual Util::Name get_profile_name() const override
        {
          return N(cp_ui_controller);
        }

        virtual void
        serialize(Util::Serial::Node &p_Node) const override;
        virtual void
        deserialize(const Util::Serial::Node &p_Node) override;
      };

      struct LOW_EDITOR_API CompileProfile
      {
        virtual ~CompileProfile() = default;

        virtual Util::Name get_name() const = 0;
        virtual std::unique_ptr<CompileProfileSettings>
        create_settings() const = 0;
        virtual void compile(Document &p_Document,
                             CompileContext &p_Context) const = 0;
      };

      struct LOW_EDITOR_API UiControllerCompileProfile
          : public CompileProfile
      {
        virtual Util::Name get_name() const override
        {
          return N(cp_ui_controller);
        }
        virtual std::unique_ptr<CompileProfileSettings>
        create_settings() const override;
        virtual Util::String
        get_class_name(const Document &p_Document) const;
        virtual void collect_entry_points(
            Graph &p_Graph,
            Util::List<CompileEntryPoint> &p_EntryPoints) const;
        virtual void emit_prelude(Graph &p_Graph,
                                  CompileContext &p_Context) const;
        virtual void emit_members(Graph &p_Graph,
                                  CompileContext &p_Context) const;
        virtual void
        emit_entry_point(Graph &p_Graph,
                         const CompileEntryPoint &p_Entry,
                         CompileContext &p_Context) const;

        virtual void
        compile(Document &p_Document,
                CompileContext &p_Context) const override;
      };

      struct LOW_EDITOR_API GameplaySystemCompileProfileSettings
          : public CompileProfileSettings
      {
        Util::String class_name = "VisualScriptGameplaySystem";

        virtual Util::Name get_profile_name() const override
        {
          return N(cp_gameplay_system);
        }

        virtual void
        serialize(Util::Serial::Node &p_Node) const override;
        virtual void
        deserialize(const Util::Serial::Node &p_Node) override;
      };

      struct LOW_EDITOR_API GameplaySystemCompileProfile
          : public CompileProfile
      {
        virtual Util::Name get_name() const override
        {
          return N(cp_gameplay_system);
        }
        virtual std::unique_ptr<CompileProfileSettings>
        create_settings() const override;
        virtual Util::String
        get_class_name(const Document &p_Document) const;
        virtual void collect_entry_points(
            Graph &p_Graph,
            Util::List<CompileEntryPoint> &p_EntryPoints) const;
        virtual void emit_members(Graph &p_Graph,
                                  CompileContext &p_Context) const;

        virtual void
        compile(Document &p_Document,
                CompileContext &p_Context) const override;
      };

      struct LOW_EDITOR_API DefaultCompileProfile
          : public CompileProfile
      {
        virtual Util::Name get_name() const override;
        virtual std::unique_ptr<CompileProfileSettings>
        create_settings() const override;
        virtual void collect_entry_points(
            Graph &p_Graph,
            Util::List<CompileEntryPoint> &p_EntryPoints) const;
        virtual void
        compile(Document &p_Document,
                CompileContext &p_Context) const override;
      };

      struct LOW_EDITOR_API CompileProfileRegistry
      {
        Util::Map<Util::Name, CompileProfile *> profiles;

        void register_profile(CompileProfile &p_Profile);
        CompileProfile *find_profile(Util::Name p_ProfileName);
        const CompileProfile *
        find_profile(Util::Name p_ProfileName) const;
      };

      LOW_EDITOR_API void register_builtin_compile_profiles(
          CompileProfileRegistry &p_ProfileRegistry);

      struct LOW_EDITOR_API Graph
      {
        NodeGraph graph;
        Util::Map<Util::Name, NodeTypeDefinition> node_types;
        Util::Map<Util::Name, NodeSpawnEntry> spawn_entries;
        Util::List<Variable> variables;
        VariableUserDataFactory variable_user_data_factory;

        void apply_context(const ScriptContextDefinition &p_Context);
        u64 id_counter = 1;

        NodeGraphMutationResult<Node>
        add_node(std::unique_ptr<Node> p_Node,
                 const NodeGraphSchema *p_Schema = nullptr);

        NodeGraphMutationResult<Pin>
        add_pin(const Pin &p_Pin,
                const NodeGraphSchema *p_Schema = nullptr);

        NodeGraphMutationResult<Low::Editor::Link>
        add_link(const Low::Editor::Link &p_Link,
                 const NodeGraphSchema *p_Schema = nullptr);

        bool remove_node(NodeId p_NodeId);
        bool remove_pin(PinId p_PinId);
        bool remove_link(LinkId p_LinkId);

        Node *find_node(NodeId p_NodeId);
        const Node *find_node(NodeId p_NodeId) const;
        Node *find_node_checked(NodeId p_NodeId);
        const Node *find_node_checked(NodeId p_NodeId) const;

        Pin *find_pin(PinId p_PinId);
        const Pin *find_pin(PinId p_PinId) const;
        Pin *find_pin_checked(PinId p_PinId);
        const Pin *find_pin_checked(PinId p_PinId) const;

        Util::List<Pin *> get_node_pins(NodeId p_NodeId);
        Util::List<const Pin *> get_node_pins(NodeId p_NodeId) const;

        Util::List<Pin *> get_input_pins(NodeId p_NodeId);
        Util::List<const Pin *> get_input_pins(NodeId p_NodeId) const;

        Util::List<Pin *> get_output_pins(NodeId p_NodeId);
        Util::List<const Pin *>
        get_output_pins(NodeId p_NodeId) const;

        Pin *find_input_pin(NodeId p_NodeId,
                            const Util::String &p_DisplayName);
        const Pin *
        find_input_pin(NodeId p_NodeId,
                       const Util::String &p_DisplayName) const;
        Pin *find_output_pin(NodeId p_NodeId,
                             const Util::String &p_DisplayName);
        const Pin *
        find_output_pin(NodeId p_NodeId,
                        const Util::String &p_DisplayName) const;

        Pin *
        find_input_pin_checked(NodeId p_NodeId,
                               const Util::String &p_DisplayName);
        const Pin *find_input_pin_checked(
            NodeId p_NodeId, const Util::String &p_DisplayName) const;
        Pin *
        find_output_pin_checked(NodeId p_NodeId,
                                const Util::String &p_DisplayName);
        const Pin *find_output_pin_checked(
            NodeId p_NodeId, const Util::String &p_DisplayName) const;

        template <typename T> T *find_node_as(NodeId p_Id)
        {
          return dynamic_cast<T *>(find_node(p_Id));
        }
        template <typename T> const T *find_node_as(NodeId p_Id) const
        {
          return dynamic_cast<const T *>(find_node(p_Id));
        }

        bool add_variable(const Variable &p_Variable);
        bool remove_variable(const Util::String &p_Name);
        Variable *find_variable(const Util::String &p_Name);
        const Variable *
        find_variable(const Util::String &p_Name) const;

        template <typename T> Util::Name register_node_type()
        {
          T l_Default;
          const Util::Name l_Name = l_Default.get_name();
          node_types[l_Name] = {l_Name,
                                []() -> std::unique_ptr<Node> {
                                  return std::make_unique<T>();
                                }};
          return l_Name;
        }
        const NodeTypeDefinition *
        find_node_type(Util::Name p_Name) const;

        void register_spawn_entry(const NodeSpawnEntry &p_SpawnEntry);
        NodeSpawnEntry *find_spawn_entry(Util::Name p_SpawnEntryId);
        const NodeSpawnEntry *
        find_spawn_entry(Util::Name p_SpawnEntryId) const;
        Util::List<const NodeSpawnEntry *> get_spawn_entries() const;

        NodeGraphMutationResult<Node> create_node(
            Util::Name p_NodeType, const Math::Vector2 &p_Position,
            const NodeGraphSchema *p_Schema = nullptr,
            const std::function<void(Node &)> &p_Initialize = {});

        NodeGraphMutationResult<Node> create_node_from_spawn_entry(
            Util::Name p_SpawnEntryId,
            const Math::Vector2 &p_Position,
            const NodeGraphSchema *p_Schema = nullptr);

        bool is_pin_connected(PinId p_PinId) const;
        PinId get_connected_pin(PinId p_PinId) const;
        Util::List<PinId> get_connected_pins(PinId p_PinId) const;

        void compile_node(NodeId p_NodeId,
                          CompileContext &p_CompileContext) const;
        void
        continue_compilation(PinId p_ExecutionOutputPinId,
                             CompileContext &p_CompileContext) const;
        void
        compile_input_pin(PinId p_InputPinId,
                          CompileContext &p_CompileContext) const;

        void serialize(Util::Serial::Node &p_Node) const;
        void deserialize(Util::Serial::Node &p_Node);

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
                       const Low::Editor::Pin &p_Pin) const override;

        virtual NodeGraphValidationResult validate_link(
            const NodeGraph &p_Graph,
            const Low::Editor::Pin &p_StartPin,
            const Low::Editor::Pin &p_EndPin) const override;

        virtual bool allows_multiple_links_per_pin(
            const NodeGraph &p_Graph,
            const Low::Editor::Pin &p_Pin) const override;

      private:
        const Graph *get_graph(const NodeGraph &p_Graph) const;
      };

      struct LOW_EDITOR_API NodeRenderer
          : public NodeGraphNodeRenderer
      {
        float pin_names_min_zoom = 0.6f;
        float default_editors_min_zoom = 0.6f;
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
                      const Low::Editor::Node &p_Node) const override;

        virtual void render_node(NodeGraphEditorContext &p_Context,
                                 Low::Editor::Node &p_Node,
                                 const ImVec2 &p_ScreenMin,
                                 const ImVec2 &p_ScreenMax) override;

        virtual bool
        get_pin_anchor(const NodeGraphEditorContext &p_Context,
                       const Low::Editor::Node &p_Node,
                       const Low::Editor::Pin &p_Pin,
                       const ImVec2 &p_ScreenMin,
                       const ImVec2 &p_ScreenMax,
                       ImVec2 &p_Anchor) const override;
      };

      struct LOW_EDITOR_API GraphRenderer
          : public NodeGraphEditorRenderer
      {
        struct SpawnAdapter : public NodeGraphSpawner
        {
          Graph *graph = nullptr;

          virtual Util::List<NodeGraphSpawnEntry> get_spawn_entries(
              NodeGraphEditorContext &p_Context) const override;

          virtual bool
          spawn_entry(NodeGraphEditorContext &p_Context,
                      Util::Name p_EntryId,
                      const Math::Vector2 &p_Position) override;
        };

        Graph *graph = nullptr;
        NodeRenderer node_renderer;
        SpawnAdapter spawn_adapter;

        GraphRenderer() = default;
        GraphRenderer(Graph &p_Graph)
            : graph(&p_Graph), node_renderer(p_Graph)
        {
          spawn_adapter.graph = &p_Graph;
        }

        void set_graph(Graph &p_Graph)
        {
          graph = &p_Graph;
          node_renderer.set_graph(p_Graph);
          spawn_adapter.graph = &p_Graph;
        }

        virtual NodeGraphNodeRenderer *
        get_node_renderer(NodeGraphEditorContext &p_Context,
                          Low::Editor::Node &p_Node) override;

        virtual NodeGraphSpawner *
        get_spawner(NodeGraphEditorContext &p_Context) override;

        virtual NodeGraphMutationResult<Low::Editor::Link>
        create_link(NodeGraphEditorContext &p_Context,
                    const Low::Editor::Link &p_Link) override;

        virtual void
        render_pin_context_menu(NodeGraphEditorContext &p_Context,
                                Low::Editor::Pin &p_Pin) override;

        virtual void
        render_node_context_menu(NodeGraphEditorContext &p_Context,
                                 Low::Editor::Node &p_Node) override;
      };

      LOW_EDITOR_API const char *pin_type_to_string(PinType p_Type);
      LOW_EDITOR_API PinType
      string_to_pin_type(const Util::String &p_Type);

      LOW_EDITOR_API PinType
      variant_type_to_pin_type(u8 p_VariantType);

      LOW_EDITOR_API u8 pin_to_variant_type(const Pin &p_Pin);

      // Returns the element type; container and concrete type IDs
      // stay separate.
      LOW_EDITOR_API Util::RTTI::PropertyType::Enum
      pin_type_to_property_type(PinType p_Type,
                                NumberSubtype p_NumberSubtype,
                                StringSubtype p_StringSubtype);

      LOW_EDITOR_API ListDefaultValue make_variable_list_default(
          PinType p_Type, NumberSubtype p_NumberSubtype,
          StringSubtype p_StringSubtype,
          Util::TypeIdentifier p_ReferencedType = {});

      LOW_EDITOR_API Util::Variant
      default_value_for_pin(const Pin &p_Pin);

      LOW_EDITOR_API Pin make_input_pin(Graph &p_Graph,
                                        NodeId p_NodeId,
                                        Pin p_Pin = {});

      LOW_EDITOR_API Pin make_output_pin(Graph &p_Graph,
                                         NodeId p_NodeId,
                                         Pin p_Pin = {});

      LOW_EDITOR_API Pin
      make_execution_pin(Util::String p_DisplayName);

      LOW_EDITOR_API Pin make_bool_pin(Util::String p_DisplayName,
                                       bool p_DefaultValue = false);

      LOW_EDITOR_API Pin make_number_pin(
          Util::String p_DisplayName,
          NumberSubtype p_NumberSubtype = NumberSubtype::Float,
          PinContainerType p_ContainerType = PinContainerType::None);

      LOW_EDITOR_API Pin make_string_pin(
          Util::String p_DisplayName,
          StringSubtype p_StringSubtype = StringSubtype::String,
          PinContainerType p_ContainerType = PinContainerType::None);

      LOW_EDITOR_API Pin make_handle_pin(
          Util::String p_DisplayName,
          Util::TypeIdentifier p_HandleType = Util::TypeIdentifier(),
          PinContainerType p_ContainerType = PinContainerType::None);

      LOW_EDITOR_API Pin make_struct_pin(
          Util::String p_DisplayName,
          Util::TypeIdentifier p_StructType = Util::TypeIdentifier(),
          PinContainerType p_ContainerType = PinContainerType::None);

      LOW_EDITOR_API Pin make_enum_pin(
          Util::String p_DisplayName,
          Util::TypeIdentifier p_EnumType = Util::TypeIdentifier(),
          PinContainerType p_ContainerType = PinContainerType::None);

      LOW_EDITOR_API Pin make_vector2_pin(
          Util::String p_DisplayName,
          PinContainerType p_ContainerType = PinContainerType::None);

      LOW_EDITOR_API Pin make_vector3_pin(
          Util::String p_DisplayName,
          PinContainerType p_ContainerType = PinContainerType::None);

      LOW_EDITOR_API Pin make_dynamic_pin(
          Util::String p_DisplayName,
          PinContainerType p_ContainerType = PinContainerType::None);

      LOW_EDITOR_API Util::String
      make_vs_identifier(const Util::String &p_Value,
                         const Util::String &p_Fallback);

      LOW_EDITOR_API Util::String pin_type_to_script_type_string(
          PinType p_Type, NumberSubtype p_NumberSubtype,
          StringSubtype p_StringSubtype,
          Util::TypeIdentifier p_HandleType);
    } // namespace VisualScript
  }   // namespace Editor
} // namespace Low
