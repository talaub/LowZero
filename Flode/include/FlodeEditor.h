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

    static void
    register_node_type(Low::Util::String p_Category,
                       Low::Util::String p_Title,
                       Flode::create_node_callback p_Callback);

  private:
    void render_graph(float p_Delta);

    void render_node(float p_Delta, const Flode::Node &p_Node);
    void render_pin(float p_Delta, const Flode::Pin &p_Pin);
    void render_link(float p_Delta, const Flode::Link &p_Link);

    bool is_pin_connected(const Flode::Pin &p_Pin);
    void collect_connected_pins();

    void create_link(NodeEd::PinId p_InputPin,
                     NodeEd::PinId p_OutputPin);

    bool can_create_link(NodeEd::PinId p_InputPin,
                         NodeEd::PinId p_OutputPin);

    void delete_link(NodeEd::LinkId p_LinkId);

    Flode::Pin find_pin(NodeEd::PinId p_PinId);

    Low::Util::List<Flode::Link> m_Links;
    Low::Util::List<Flode::Node> m_Nodes;

    Low::Util::Set<u64> m_ConnectedPins;

    NodeEd::EditorContext *m_Context;

    static Low::Util::Map<Low::Util::String,
                          Low::Util::Map<Low::Util::String,
                                         Flode::create_node_callback>>
        ms_NodeTypes;
  };
} // namespace Flode
