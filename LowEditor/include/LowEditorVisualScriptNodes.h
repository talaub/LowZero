#pragma once

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
        LOW_EDITOR_API void register_nodes(Graph &p_Graph);
      }

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
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
