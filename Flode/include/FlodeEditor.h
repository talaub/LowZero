#pragma once

#include "FlodeApi.h"

#include <imgui_node_editor.h>

#include "LowUtilContainers.h"

#include "LowMath.h"

#include "Flode.h"

namespace Flode {
  struct FLODE_API Editor
  {
  public:
    Editor();
    void render(float p_Delta);

    void load(Low::Util::String p_Path);

  private:
    void render_graph(float p_Delta);

    void render_link(float p_Delta, const Flode::Link *p_Link);

    void render_data_panel();

    void create_node_popup();

    NodeEd::EditorContext *m_Context;

    Graph *m_Graph;

    ImVec2 m_StoredMousePosition;

    bool m_FirstRun;

    Low::Util::List<Node *> m_SelectedNodes;
  };
} // namespace Flode
