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
        struct LOW_EDITOR_API GlobalFunctionCallNodeData
            : public NodeUserData
        {
          Core::Scripting::FunctionInfo function_info;
        };

        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      }

      namespace EnumNodes {
        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      } // namespace EnumNodes

      namespace StructNodes {
        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      } // namespace StructNodes

      namespace UiControllerNodes {
        enum class InteractionType
        {
          Click,
          MouseEnter,
          MouseExit
        };

        struct LOW_EDITOR_API ElementEventNodeData
            : public NodeUserData
        {
          Util::Name element_name;
          u64 element_local_id = 0;
          InteractionType interaction_type = InteractionType::Click;
        };

        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      } // namespace UiControllerNodes

      namespace GameplaySystemNodes {
        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      } // namespace GameplaySystemNodes

      namespace FunctionNodes {
        struct LOW_EDITOR_API CallFunctionNodeData : public NodeUserData
        {
          Util::String function_name;
        };

        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      } // namespace FunctionNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
