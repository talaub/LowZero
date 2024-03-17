#include "LowCoreUi.h"

namespace Low {
  namespace Core {
    namespace UI {
      Element g_HoveredElement;

      void set_hovered_element(Element p_Element)
      {
        g_HoveredElement = p_Element;
      }

      Element get_hovered_element()
      {
        return g_HoveredElement;
      }
    } // namespace UI
  }   // namespace Core
} // namespace Low
