#pragma once

#include "LowCoreScripting.h"
#include "LowEditorVisualScripting.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace BoolNodes {
        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      }

      namespace CastNodes {
        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      }

      namespace DebugNodes {
        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      }

      namespace HandleNodes {
        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      }

      namespace MathNodes {
        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      }

      namespace OperatorNodes {
        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      }

      namespace SyntaxNodes {
        struct LOW_EDITOR_API GlobalFunctionCallNode : public Node
        {
          Util::String function_name;
          Core::Scripting::FunctionInfo function_info;
          Util::Name get_name() const override;
          Util::String get_title(const Graph &p_Graph) const override;
          Util::String
          get_subtitle(const Graph &p_Graph) const override;
          Util::String
          get_category(const Graph &p_Graph) const override;
          Util::String get_icon(const Graph &p_Graph) const override;
          ImU32 get_color(const Graph &) const override;
          void setup_default_pins(
              Graph &p_Graph,
              const NodeGraphSchema *p_Schema) const override;
          void
          compile(Graph &p_Graph,
                  CompileContext &p_CompileContext) const override;
          void serialize(Util::Serial::Node &p_Data) const override;
          void deserialize(const Util::Serial::Node &p_Data) override;
        };

        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      } // namespace SyntaxNodes

      namespace EnumNodes {
        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      } // namespace EnumNodes

      namespace StructNodes {
        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      } // namespace StructNodes

      namespace ArrayNodes {
        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      } // namespace ArrayNodes

      namespace UiControllerNodes {
        enum class InteractionType
        {
          Click,
          MouseEnter,
          MouseExit
        };

        struct LOW_EDITOR_API ElementEventNode : public Node
        {
          Util::Name element_name;
          u64 element_local_id = 0;
          InteractionType interaction_type = InteractionType::Click;
          Util::Name get_name() const override;
          Util::String get_title(const Graph &p_Graph) const override;
          Util::String get_subtitle(const Graph &) const override;
          Util::String get_category(const Graph &) const override;
          Util::String get_icon(const Graph &) const override;
          ImU32 get_color(const Graph &) const override;
          void setup_default_pins(
              Graph &p_Graph,
              const NodeGraphSchema *p_Schema) const override;
          void
          compile(Graph &p_Graph,
                  CompileContext &p_CompileContext) const override;
          void serialize(Util::Serial::Node &p_Data) const override;
          void deserialize(const Util::Serial::Node &p_Data) override;
        };

        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      } // namespace UiControllerNodes

      namespace GameplaySystemNodes {
        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      } // namespace GameplaySystemNodes

      namespace FunctionNodes {
        struct LOW_EDITOR_API CallFunctionNode : public Node
        {
          Util::String function_name;
          Util::Name get_name() const override;
          Util::String get_title(const Graph &p_Graph) const override;
          Util::String get_subtitle(const Graph &) const override;
          Util::String get_category(const Graph &) const override;
          Util::String get_icon(const Graph &) const override;
          ImU32 get_color(const Graph &) const override;
          void setup_default_pins(
              Graph &p_Graph,
              const NodeGraphSchema *p_Schema) const override;
          void
          compile(Graph &p_Graph,
                  CompileContext &p_CompileContext) const override;
          void serialize(Util::Serial::Node &p_Data) const override;
          void deserialize(const Util::Serial::Node &p_Data) override;
        };

        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      } // namespace FunctionNodes

      namespace SignalNodes {
        struct LOW_EDITOR_API MakeSignalNode : public Node
        {
          Util::String function_name;
          Util::Name get_name() const override;
          Util::String get_title(const Graph &p_Graph) const override;
          Util::String get_subtitle(const Graph &) const override;
          Util::String get_category(const Graph &) const override;
          Util::String get_icon(const Graph &) const override;
          ImU32 get_color(const Graph &) const override;
          void setup_default_pins(
              Graph &p_Graph,
              const NodeGraphSchema *p_Schema) const override;
          void compile_output_pin(
              Graph &p_Graph, PinId p_PinId,
              CompileContext &p_CompileContext) const override;
          void serialize(Util::Serial::Node &p_Data) const override;
          void deserialize(const Util::Serial::Node &p_Data) override;
        };

        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      } // namespace SignalNodes
    }   // namespace VisualScript
  }     // namespace Editor
} // namespace Low
