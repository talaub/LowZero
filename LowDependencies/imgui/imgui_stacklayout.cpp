// dear imgui, v1.86 WIP
// (stack layout code)

/*

Index of this file:

// [SECTION] Commentary
// [SECTION] Header mess
// [SECTION] Stack layout: Forward declarations
// [SECTION] Stack layout: flags, enums, data structures
// [SECTION] Stack Layout: Context forward declarations
// [SECTION] Stack Layout: Internal code forward declarations
// [SECTION] Stack Layout: Context
// [SECTION] Stack Layout: Internal code
// [SECTION] Stack Layout: Internal API
// [SECTION] Stack Layout: Public API

*/

// Navigating this file:
// - In Visual Studio IDE: CTRL+comma ("Edit.NavigateTo") can follow
// symbols in comments, whereas CTRL+F12 ("Edit.GoToImplementation")
// cannot.
// - With Visual Assist installed: ALT+G
// ("VAssistX.GoToImplementation") can also follow symbols in
// comments.

//-----------------------------------------------------------------------------
// [SECTION] Commentary
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Typical call flow: (root level is generally public API):
//-----------------------------------------------------------------------------
//
// - BeginHorizontal/BeginVertical()            user begin into a
// layout
// - [...]                                      user emit contents
// -  | Spring()                                - separate widget
// groups in layout
// -  | SuspendLayout()                         - stop any layout
// operations
// -  | ResumeLayout()                          - resume layout
// operations
//-----------------------------------------------------------------------------
// - EndHorizontal/EndVertical()                user ends the layout
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// [SECTION] Header mess
//-----------------------------------------------------------------------------

// dear imgui (docking) — stack layout implementation (updated
// for 1.89/1.90+)

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_internal.h"
#include "imgui_stacklayout_internal.h"

#include <stdint.h>

// --------------------------------------------------------
// Small compatibility helpers (changed internals across versions)
// --------------------------------------------------------
#if !defined(IMGUI_VERSION_NUM)
// Fallback, but docking era should define IMGUI_VERSION_NUM
#define IMGUI_VERSION_NUM 18900
#endif

// Access current vtx index robustly (avoid direct use of
// _VtxCurrentIdx)
static inline unsigned int ImDrawList_GetCurrentVtxIndex(ImDrawList *dl) {
  // In 1.89/1.90+, VtxBuffer.Size is a safe stand-in for current
  // vertex write index. (In older code `_VtxCurrentIdx` was used;
  // it's intentionally considered internal.)
  return (unsigned int)dl->VtxBuffer.Size;
}

// Safer PushID for ImGuiID: use PushOverrideID which is stable in
// docking branch.
static inline void PushID_ImGuiID(ImGuiID id) {
#if IMGUI_VERSION_NUM >= 18900
  ImGui::PushOverrideID(id);
#else
  // Fallback (older) – not expected on docking builds but kept for
  // completeness
  ImGui::PushID((void *)(intptr_t)id);
#endif
}

// --------------------------------------------------------
// Types
// --------------------------------------------------------
typedef int ImGuiLayoutItemType;
enum ImGuiLayoutItemType_ {
  ImGuiLayoutItemType_Item,
  ImGuiLayoutItemType_Spring
};

struct ImGuiLayoutItem {
  ImGuiLayoutItemType Type;
  ImRect MeasuredBounds;
  float SpringWeight;
  float SpringSpacing;
  float SpringSize;
  float CurrentAlign;
  float CurrentAlignOffset;
  unsigned int VertexIndexBegin;
  unsigned int VertexIndexEnd;

  ImGuiLayoutItem(ImGuiLayoutItemType type)
      : Type(type), MeasuredBounds(0, 0, 0, 0), SpringWeight(1.0f),
        SpringSpacing(-1.0f), SpringSize(0.0f), CurrentAlign(0.0f),
        CurrentAlignOffset(0.0f), VertexIndexBegin(0), VertexIndexEnd(0) {}
};

struct ImGuiLayout {
  ImGuiID Id;
  ImGuiLayoutType Type;
  bool Live;
  ImVec2 Size;
  ImVec2 CurrentSize;
  ImVec2 MinimumSize;
  ImVec2 MeasuredSize;

  ImVector<ImGuiLayoutItem> Items;
  int CurrentItemIndex;
  int ParentItemIndex;
  ImGuiLayout *Parent;
  ImGuiLayout *FirstChild;
  ImGuiLayout *NextSibling;
  float Align;
  float Indent;
  ImVec2 StartPos;
  ImVec2 StartCursorMaxPos;

  ImGuiLayoutType PrevWindowLayoutType;

  ImGuiLayout(ImGuiID id, ImGuiLayoutType type)
      : Id(id), Type(type), Live(false), Size(0, 0), CurrentSize(0, 0),
        MinimumSize(0, 0), MeasuredSize(0, 0), CurrentItemIndex(0),
        ParentItemIndex(0), Parent(nullptr), FirstChild(nullptr),
        NextSibling(nullptr), Align(-1.0f), Indent(0.0f), StartPos(0, 0),
        StartCursorMaxPos(0, 0),
        PrevWindowLayoutType(ImGuiLayoutType_Vertical) {}
};

struct ImGuiLayoutWindowState {
  ImGuiWindow *Window;
  ImGuiLayout *CurrentLayout;
  ImGuiLayoutItem *CurrentLayoutItem;
  ImVector<ImGuiLayout *> LayoutStack;
  ImGuiStorage Layouts;

  ImGuiLayoutWindowState()
      : Window(nullptr), CurrentLayout(nullptr), CurrentLayoutItem(nullptr) {}
};

struct ImGuiLayoutState {
  ImGuiStorage LayoutWindowStates;
  ImGuiContext *Context;
  ImGuiID ContextID;
  ImGuiID NewFramePreID;
  ImGuiID EndFramePreID;
  ImGuiID ShutdownHookID;

  ImGuiLayoutState()
      : Context(nullptr), ContextID(0), NewFramePreID(0), EndFramePreID(0),
        ShutdownHookID(0) {}
};

// --------------------------------------------------------
// Context
// --------------------------------------------------------
static ImGuiStorage GStackLayoutStates;

static ImGuiID get_context_id(ImGuiContext *context) {
  // Hash pointer to context (stable & unique per-context)
#if IMGUI_VERSION_NUM >= 18900
  return ImHashData(&context, sizeof(context), 0);
#else
  return ImHashData(&context, sizeof(context));
#endif
}

static ImGuiLayoutState *get_layout_state(ImGuiContext *context) {
  if (!context)
    return nullptr;
  ImGuiID l_ContextId = get_context_id(context);
  ImGuiLayoutState *l_State =
      (ImGuiLayoutState *)GStackLayoutStates.GetVoidPtr(l_ContextId);
  if (!l_State) {
    l_State = IM_NEW(ImGuiLayoutState);
    l_State->Context = context;
    l_State->ContextID = l_ContextId;

    ImGuiContextHook new_frame_pre_hook{};
    new_frame_pre_hook.Type = ImGuiContextHookType_NewFramePre;
    new_frame_pre_hook.UserData = l_State;
    new_frame_pre_hook.Callback = [](ImGuiContext *ctx,
                                     ImGuiContextHook *hook) {
      ImGuiLayoutState *layout_state = (ImGuiLayoutState *)hook->UserData;
      for (int i = 0; i < layout_state->LayoutWindowStates.Data.Size; ++i) {
        auto *ws =
            (ImGuiLayoutWindowState *)layout_state->LayoutWindowStates.Data[i]
                .val_p;
        // mark layouts as not-live this frame
        for (int j = 0; j < ws->Layouts.Data.Size; ++j)
          ((ImGuiLayout *)ws->Layouts.Data[j].val_p)->Live = false;
      }
    };
    l_State->NewFramePreID =
        ImGui::AddContextHook(context, &new_frame_pre_hook);

    ImGuiContextHook end_frame_pre_hook{};
    end_frame_pre_hook.Type = ImGuiContextHookType_EndFramePre;
    end_frame_pre_hook.UserData = l_State;
    end_frame_pre_hook.Callback = [](ImGuiContext *ctx,
                                     ImGuiContextHook *hook) {
      ImGuiLayoutState *layout_state = (ImGuiLayoutState *)hook->UserData;
      for (int i = 0; i < layout_state->LayoutWindowStates.Data.Size; ++i) {
        auto *ws =
            (ImGuiLayoutWindowState *)layout_state->LayoutWindowStates.Data[i]
                .val_p;
        IM_ASSERT(
            (ws->LayoutStack.Size == 0 ||
             ws->LayoutStack.back()->Type == ImGuiLayoutType_Horizontal) &&
            "BeginHorizontal/EndHorizontal mismatch!");
        IM_ASSERT((ws->LayoutStack.Size == 0 ||
                   ws->LayoutStack.back()->Type == ImGuiLayoutType_Vertical) &&
                  "BeginVertical/EndVertical mismatch!");
      }
    };
    l_State->EndFramePreID =
        ImGui::AddContextHook(context, &end_frame_pre_hook);

    ImGuiContextHook shutdown_hook{};
    shutdown_hook.Type = ImGuiContextHookType_Shutdown;
    shutdown_hook.UserData = l_State;
    shutdown_hook.Callback = [](ImGuiContext *ctx, ImGuiContextHook *hook) {
      ImGuiLayoutState *layout_state = (ImGuiLayoutState *)hook->UserData;
      for (int i = 0; i < layout_state->LayoutWindowStates.Data.Size; ++i) {
        auto *ws =
            (ImGuiLayoutWindowState *)layout_state->LayoutWindowStates.Data[i]
                .val_p;
        for (int j = 0; j < ws->Layouts.Data.Size; ++j)
          IM_DELETE((ImGuiLayout *)ws->Layouts.Data[j].val_p);
        IM_DELETE(ws);
      }
      GStackLayoutStates.SetVoidPtr(layout_state->ContextID, nullptr);
      IM_DELETE(layout_state);
    };
    l_State->ShutdownHookID = ImGui::AddContextHook(context, &shutdown_hook);

    GStackLayoutStates.SetVoidPtr(l_ContextId, l_State);
  }
  return l_State;
}

static ImGuiLayoutWindowState *
get_window_layout_state(ImGuiID window_id, ImGuiWindow *window = nullptr) {
  ImGuiLayoutState *l_State = get_layout_state(ImGui::GetCurrentContext());
  if (!l_State)
    return nullptr;

  auto *ws = (ImGuiLayoutWindowState *)l_State->LayoutWindowStates.GetVoidPtr(
      window_id);
  if (!ws) {
    if (!window) {
      window = ImGui::FindWindowByID(window_id);
      IM_ASSERT(window && "get_window_layout_state: invalid window id");
    }
    ws = IM_NEW(ImGuiLayoutWindowState);
    ws->Window = window;
    l_State->LayoutWindowStates.SetVoidPtr(window_id, ws);
  }
  return ws;
}

static ImGuiLayoutWindowState *get_current_window_layout_state() {
  ImGuiWindow *w = ImGui::GetCurrentWindow();
  if (!w)
    return nullptr;
  return get_window_layout_state(w->ID, w);
}

// --------------------------------------------------------
// Internal layout helpers
// --------------------------------------------------------
namespace ImGui {
static ImGuiLayout *find_layout(ImGuiID id, ImGuiLayoutType type) {
  IM_ASSERT(type == ImGuiLayoutType_Horizontal ||
            type == ImGuiLayoutType_Vertical);
  auto *ws = get_current_window_layout_state();
  auto *layout = (ImGuiLayout *)ws->Layouts.GetVoidPtr(id);
  if (!layout)
    return nullptr;
  if (layout->Type != type) {
    layout->Type = type;
    layout->MinimumSize = ImVec2(0, 0);
    layout->Items.clear();
  }
  return layout;
}

static ImGuiLayout *create_layout(ImGuiID id, ImGuiLayoutType type,
                                  ImVec2 size) {
  auto *ws = get_current_window_layout_state();
  auto *layout = IM_NEW(ImGuiLayout)(id, type);
  layout->Size = size;
  ws->Layouts.SetVoidPtr(id, layout);
  return layout;
}

static void push_layout(ImGuiLayout *layout) {
  auto *ws = get_current_window_layout_state();
  if (layout) {
    layout->Parent = ws->CurrentLayout;
    if (layout->Parent) {
      layout->ParentItemIndex = layout->Parent->CurrentItemIndex;
      layout->NextSibling = ws->CurrentLayout->FirstChild;
      layout->FirstChild = nullptr;
      ws->CurrentLayout->FirstChild = layout;
    } else {
      layout->NextSibling = nullptr;
      layout->FirstChild = nullptr;
    }
  }
  ws->LayoutStack.push_back(layout);
  ws->CurrentLayout = layout;
  ws->CurrentLayoutItem = nullptr;
}

static void pop_layout(ImGuiLayout *layout) {
  auto *ws = get_current_window_layout_state();
  IM_ASSERT(!ws->LayoutStack.empty() && ws->LayoutStack.back() == layout);
  ws->LayoutStack.pop_back();
  if (!ws->LayoutStack.empty()) {
    ws->CurrentLayout = ws->LayoutStack.back();
    ws->CurrentLayoutItem =
        &ws->CurrentLayout->Items[ws->CurrentLayout->CurrentItemIndex];
  } else {
    ws->CurrentLayout = nullptr;
    ws->CurrentLayoutItem = nullptr;
  }
}

static ImGuiLayoutItem *gen_item(ImGuiLayout &layout,
                                 ImGuiLayoutItemType type) {
  auto *ws = get_current_window_layout_state();
  if (layout.CurrentItemIndex < layout.Items.Size) {
    ImGuiLayoutItem &it = layout.Items[layout.CurrentItemIndex];
    if (it.Type != type)
      it = ImGuiLayoutItem(type);
  } else {
    layout.Items.push_back(ImGuiLayoutItem(type));
  }
  ws->CurrentLayoutItem = &layout.Items[layout.CurrentItemIndex];
  return ws->CurrentLayoutItem;
}

static void translate_item(ImGuiLayoutItem &item, const ImVec2 &offset) {
  if ((offset.x == 0.0f && offset.y == 0.0f) ||
      (item.VertexIndexBegin == item.VertexIndexEnd))
    return;
  ImDrawList *dl = GetWindowDrawList();
  ImDrawVert *begin = dl->VtxBuffer.Data + item.VertexIndexBegin;
  ImDrawVert *end = dl->VtxBuffer.Data + item.VertexIndexEnd;
  for (ImDrawVert *vtx = begin; vtx < end; ++vtx) {
    vtx->pos.x += offset.x;
    vtx->pos.y += offset.y;
  }
}

static inline void signed_indent(float indent) {
  if (indent > 0.0f)
    ImGui::Indent(indent);
  else if (indent < 0.0f)
    ImGui::Unindent(-indent);
}

static float calc_item_align_offset(ImGuiLayout &layout,
                                    ImGuiLayoutItem &item) {
  if (item.CurrentAlign <= 0.0f)
    return 0.0f;
  const ImVec2 size = item.MeasuredBounds.GetSize();
  const float layout_extent = (layout.Type == ImGuiLayoutType_Horizontal)
                                  ? layout.CurrentSize.y
                                  : layout.CurrentSize.x;
  const float item_extent =
      (layout.Type == ImGuiLayoutType_Horizontal) ? size.y : size.x;
  if (item_extent <= 0.0f)
    return 0.0f;
  return ImFloor(item.CurrentAlign * (layout_extent - item_extent));
}

static ImVec2 balance_item_align(ImGuiLayout &layout, ImGuiLayoutItem &item) {
  ImVec2 delta(0, 0);
  if (item.CurrentAlign > 0.0f) {
    const float want = calc_item_align_offset(layout, item);
    if (want != item.CurrentAlignOffset) {
      const float d = want - item.CurrentAlignOffset;
      if (layout.Type == ImGuiLayoutType_Horizontal)
        delta.y = d;
      else
        delta.x = d;
      translate_item(item, delta);
      item.CurrentAlignOffset = want;
    }
  }
  return delta;
}

static void balance_items_align(ImGuiLayout &layout) {
  for (int i = 0; i < layout.Items.Size; ++i)
    (void)balance_item_align(layout, layout.Items[i]);
}

static bool has_nonzero_spring(ImGuiLayout &layout) {
  for (int i = 0; i < layout.Items.Size; ++i)
    if (layout.Items[i].Type == ImGuiLayoutItemType_Spring &&
        layout.Items[i].SpringWeight > 0.0f)
      return true;
  return false;
}

static ImVec2 calc_layout_size(ImGuiLayout &layout, bool collapse_springs) {
  ImVec2 bounds(0, 0);
  if (layout.Type == ImGuiLayoutType_Vertical) {
    for (int i = 0; i < layout.Items.Size; ++i) {
      auto &it = layout.Items[i];
      const ImVec2 sz = it.MeasuredBounds.GetSize();
      if (it.Type == ImGuiLayoutItemType_Item) {
        bounds.x = ImMax(bounds.x, sz.x);
        bounds.y += sz.y;
      } else {
        bounds.y += ImFloor(it.SpringSpacing);
        if (!collapse_springs)
          bounds.y += it.SpringSize;
      }
    }
  } else {
    for (int i = 0; i < layout.Items.Size; ++i) {
      auto &it = layout.Items[i];
      const ImVec2 sz = it.MeasuredBounds.GetSize();
      if (it.Type == ImGuiLayoutItemType_Item) {
        bounds.x += sz.x;
        bounds.y = ImMax(bounds.y, sz.y);
      } else {
        bounds.x += ImFloor(it.SpringSpacing);
        if (!collapse_springs)
          bounds.x += it.SpringSize;
      }
    }
  }
  return bounds;
}

static void balance_springs(ImGuiLayout &layout) {
  float total_w = 0.0f;
  int last_spring = -1;
  for (int i = 0; i < layout.Items.Size; ++i) {
    auto &it = layout.Items[i];
    if (it.Type != ImGuiLayoutItemType_Spring)
      continue;
    total_w += it.SpringWeight;
    last_spring = i;
  }

#if 0
  const bool is_h = (layout.Type == ImGuiLayoutType_Horizontal);
  const bool is_auto = (((is_h ? layout.Size.x : layout.Size.y) <= 0.0f) &&
                        layout.Parent == nullptr);
  const float occupied = is_h ? layout.MinimumSize.x : layout.MinimumSize.y;
  const float available =
      is_auto ? occupied : (is_h ? layout.CurrentSize.x : layout.CurrentSize.y);
  const float free = ImMax(available - occupied, 0.0f);
#else
  const bool is_h = (layout.Type == ImGuiLayoutType_Horizontal);
  const bool is_top_level_auto =
      ((is_h ? layout.Size.x : layout.Size.y) <= 0.0f) &&
      (layout.Parent == nullptr);

  // Space already taken by fixed items (what you already had)
  const float occupied = is_h ? layout.MinimumSize.x : layout.MinimumSize.y;

  // How much space can we actually use?
  float available;
  if (is_top_level_auto) {
    // For a top-level, auto-sized layout, use the *remaining* content space.
    // GetContentRegionAvail() is the remaining space from the current cursor
    // to the content max on this axis (after anything already submitted).
    const ImVec2 avail = ImFloor(ImGui::GetContentRegionAvail());
    const float remain = is_h ? avail.x : avail.y;

    // Total available = what’s already occupied by measured items + what
    // remains.
    available = occupied + ImMax(remain, 0.0f);

    available = occupied;

  } else {
    // For explicit-size layouts or nested layouts, use the current size you
    // computed.
    available = is_h ? layout.CurrentSize.x : layout.CurrentSize.y;
  }

  const float free = ImMax(available - occupied, 0.0f);
#endif

  float span_start = 0.0f, cur_w = 0.0f;
  for (int i = 0; i < layout.Items.Size; ++i) {
    auto &it = layout.Items[i];
    if (it.Type != ImGuiLayoutItemType_Spring)
      continue;

    const float last = it.SpringSize;
    if (free > 0.0f && total_w > 0.0f) {
      const float next_w = cur_w + it.SpringWeight;
      const float span_end =
          ImFloor((i == last_spring) ? free : (free * next_w / total_w));
      it.SpringSize = span_end - span_start;
      span_start = span_end;
      cur_w = next_w;
    } else
      it.SpringSize = 0.0f;

    if (last != it.SpringSize) {
      const float d = it.SpringSize - last;
      const ImVec2 off = is_h ? ImVec2(d, 0) : ImVec2(0, d);
      it.MeasuredBounds.Max += off;
      for (int j = i + 1; j < layout.Items.Size; ++j) {
        auto &after = layout.Items[j];
        translate_item(after, off);
        after.MeasuredBounds.Min += off;
        after.MeasuredBounds.Max += off;
      }
    }
  }
}

static inline void reset_line_state(ImGuiWindow *window) {
  // Reset per-line bookkeeping so manual CursorPos changes don’t
  // confuse ImGui into starting a new visual line.
  window->DC.CursorPosPrevLine = window->DC.CursorPos;
  window->DC.PrevLineSize = ImVec2(0.0f, 0.0f);
  window->DC.CurrLineSize = ImVec2(0.0f, 0.0f);
  window->DC.CurrLineTextBaseOffset = 0.0f;
  window->DC.PrevLineTextBaseOffset = 0.0f;
  window->DC.IsSameLine = false;
}

static void balance_children(ImGuiLayout &layout) {
  for (ImGuiLayout *child = layout.FirstChild; child;
       child = child->NextSibling) {
    if (child->Type == ImGuiLayoutType_Horizontal && child->Size.x <= 0.0f)
      child->CurrentSize.x = layout.CurrentSize.x;
    else if (child->Type == ImGuiLayoutType_Vertical && child->Size.y <= 0.0f)
      child->CurrentSize.y = layout.CurrentSize.y;

    balance_children(*child);

    if (has_nonzero_spring(*child)) {
      ImGuiLayoutItem &parent_item = layout.Items[child->ParentItemIndex];
      if (child->Type == ImGuiLayoutType_Horizontal && child->Size.x <= 0.0f)
        parent_item.MeasuredBounds.Max.x =
            ImMax(parent_item.MeasuredBounds.Max.x,
                  parent_item.MeasuredBounds.Min.x + layout.CurrentSize.x);
      else if (child->Type == ImGuiLayoutType_Vertical && child->Size.y <= 0.0f)
        parent_item.MeasuredBounds.Max.y =
            ImMax(parent_item.MeasuredBounds.Max.y,
                  parent_item.MeasuredBounds.Min.y + layout.CurrentSize.y);
    }
  }
  balance_springs(layout);
  balance_items_align(layout);
}

static void begin_layout(ImGuiID id, ImGuiLayoutType type, ImVec2 size,
                         float align) {
  ImGuiWindow *window = GetCurrentWindow();

  PushID_ImGuiID(id); // safer than PushID(id) for raw ImGuiID

  ImGuiLayout *layout = find_layout(id, type);
  if (!layout)
    layout = create_layout(id, type, size);
  layout->Live = true;
  push_layout(layout);

  if (layout->Size.x != size.x || layout->Size.y != size.y)
    layout->Size = size;

  layout->Align = (align < 0.0f) ? -1.0f : ImClamp(align, 0.0f, 1.0f);

  layout->CurrentItemIndex = 0;
  layout->CurrentSize.x =
      layout->Size.x > 0.0f ? layout->Size.x : layout->MinimumSize.x;
  layout->CurrentSize.y =
      layout->Size.y > 0.0f ? layout->Size.y : layout->MinimumSize.y;

  /*
  {
    const ImVec2 avail = ImFloor(
        ImGui::GetContentRegionAvail()); // robust to columns/child windows
    if (layout->Size.x <= 0.0f && type == ImGuiLayoutType_Horizontal)
      layout->CurrentSize.x = ImMax(layout->CurrentSize.x, avail.x);
    if (layout->Size.y <= 0.0f && type == ImGuiLayoutType_Vertical)
      layout->CurrentSize.y = ImMax(layout->CurrentSize.y, avail.y);
  }
  */

  layout->StartPos = window->DC.CursorPos;
  layout->StartCursorMaxPos = window->DC.CursorMaxPos;

  layout->PrevWindowLayoutType = window->DC.LayoutType;
  window->DC.LayoutType = type;

  if (type == ImGuiLayoutType_Vertical) {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::Dummy(ImVec2(0, 0));
    ImGui::PopStyleVar();

    layout->Indent = layout->StartPos.x - window->DC.CursorPos.x;
    signed_indent(layout->Indent);

    reset_line_state(window);
  } else // ImGuiLayoutType_Horizontal
  {
    // Same rationale as vertical: submit a zero-sized item so extending
    // boundary is legal on 1.89+ AND so line metrics start clean.
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::Dummy(ImVec2(0, 0));
    ImGui::PopStyleVar();

    reset_line_state(window);
  }

  // Begin first item
  ImGuiLayoutItem &item = *gen_item(*layout, ImGuiLayoutItemType_Item);
  item.CurrentAlign = (layout->Align < 0.0f)
                          ? 0.5f
                          : layout->Align; // default center if not provided
  item.CurrentAlign = ImClamp(item.CurrentAlign, 0.0f, 1.0f);
  item.CurrentAlignOffset = 0.0f;

  // Pre-align for previous-frame alignment (will be corrected at
  // end if needed)
  if (item.CurrentAlign > 0.0f) {
    const float off = item.CurrentAlignOffset =
        calc_item_align_offset(*layout, item);
    if (type == ImGuiLayoutType_Horizontal)
      window->DC.CursorPos.y += off;
    else {
      signed_indent(off);
      window->DC.CursorPos.x += off;
    }
  }

  item.MeasuredBounds.Min = item.MeasuredBounds.Max = window->DC.CursorPos;
  item.VertexIndexBegin = ImDrawList_GetCurrentVtxIndex(window->DrawList);
  item.VertexIndexEnd = item.VertexIndexBegin;
}

static void end_layout(ImGuiLayoutType type) {
  auto *ws = get_current_window_layout_state();
  IM_ASSERT(ws && ws->CurrentLayout && ws->CurrentLayout->Type == type);
  ImGuiLayout *layout = ws->CurrentLayout;
  ImGuiWindow *window = ws->Window;

  // End current item (flush geometry bounds)
  {
    ImGuiLayoutItem &item = layout->Items[layout->CurrentItemIndex];
    item.VertexIndexEnd = ImDrawList_GetCurrentVtxIndex(window->DrawList);

    if (item.CurrentAlign > 0.0f && layout->Type == ImGuiLayoutType_Vertical)
      signed_indent(-item.CurrentAlignOffset);

    const ImVec2 fix = balance_item_align(*layout, item);
    item.MeasuredBounds.Min += fix;
    item.MeasuredBounds.Max += fix;

    if (layout->Type == ImGuiLayoutType_Horizontal) {
      window->DC.CursorPos.y = layout->StartPos.y;
      reset_line_state(window); // <-- add this
    } else {
      window->DC.CursorPos.x = layout->StartPos.x;
      reset_line_state(window); // <-- add this (good hygiene for vertical too)
    }

    layout->CurrentItemIndex++;
  }

  if (layout->CurrentItemIndex < layout->Items.Size)
    layout->Items.resize(layout->CurrentItemIndex);

  if (layout->Type == ImGuiLayoutType_Vertical)
    signed_indent(-layout->Indent);

  pop_layout(layout);

  const bool auto_w = (layout->Size.x <= 0.0f);
  const bool auto_h = (layout->Size.y <= 0.0f);

  ImVec2 new_size = layout->Size;
  if (auto_w)
    new_size.x = layout->CurrentSize.x;
  if (auto_h)
    new_size.y = layout->CurrentSize.y;

  const ImVec2 min_size = calc_layout_size(*layout, /*collapse_springs*/ true);
  if (min_size.x != layout->MinimumSize.x ||
      min_size.y != layout->MinimumSize.y) {
    layout->MinimumSize = min_size;
    if (auto_w)
      new_size.x = min_size.x;
    if (auto_h)
      new_size.y = min_size.y;
  }

  if (!auto_w)
    new_size.x = layout->Size.x;
  if (!auto_h)
    new_size.y = layout->Size.y;

  layout->CurrentSize = new_size;

  ImVec2 measured = new_size;
  if ((auto_w || auto_h) && layout->Parent) {
    if (layout->Type == ImGuiLayoutType_Horizontal && auto_w &&
        layout->Parent->CurrentSize.x > 0)
      layout->CurrentSize.x = layout->Parent->CurrentSize.x;
    else if (layout->Type == ImGuiLayoutType_Vertical && auto_h &&
             layout->Parent->CurrentSize.y > 0)
      layout->CurrentSize.y = layout->Parent->CurrentSize.y;

    balance_springs(*layout);
    measured = layout->CurrentSize;
  }
  layout->CurrentSize = new_size;

  ImVec2 current_layout_item_max(0, 0);
  if (ws->CurrentLayoutItem)
    current_layout_item_max = ImMax(ws->CurrentLayoutItem->MeasuredBounds.Max,
                                    layout->StartPos + new_size);

  window->DC.CursorPos = layout->StartPos;
  window->DC.CursorMaxPos = layout->StartCursorMaxPos;
  reset_line_state(window);

  window->DC.LayoutType = layout->PrevWindowLayoutType;

  ImGui::ItemSize(new_size);
  ImGui::ItemAdd(ImRect(layout->StartPos, layout->StartPos + measured), 0);

  if (ws->CurrentLayoutItem) {
    // keep whatever ItemAdd() + UpdateItemRect() established,
    // only extend if our precomputed value is larger
    ws->CurrentLayoutItem->MeasuredBounds.Max = ImMax(
        ws->CurrentLayoutItem->MeasuredBounds.Max, current_layout_item_max);
  }

  if (layout->Parent == nullptr)
    balance_children(*layout);

  ImGui::PopID(); // matches PushOverrideID
}

static void add_spring(ImGuiLayout &layout, float weight, float spacing) {
  ImGuiWindow *window = GetCurrentWindow();

  // Undo padding so the spring eats the gap
  {
    ImGuiLayoutItem *prev = &layout.Items[layout.CurrentItemIndex];
    if (layout.Type == ImGuiLayoutType_Horizontal) {
      window->DC.CursorPos.x = prev->MeasuredBounds.Max.x;
      reset_line_state(window);
    } else {
      window->DC.CursorPos.y = prev->MeasuredBounds.Max.y;
      reset_line_state(window);
    }
  }

  // Finish previous item
  {
    ImGuiLayoutItem &item = layout.Items[layout.CurrentItemIndex];
    item.VertexIndexEnd = ImDrawList_GetCurrentVtxIndex(window->DrawList);

    if (item.CurrentAlign > 0.0f && layout.Type == ImGuiLayoutType_Vertical)
      signed_indent(-item.CurrentAlignOffset);

    const ImVec2 fix = balance_item_align(layout, item);
    item.MeasuredBounds.Min += fix;
    item.MeasuredBounds.Max += fix;

    if (layout.Type == ImGuiLayoutType_Horizontal) {
      window->DC.CursorPos.y = layout.StartPos.y;
      reset_line_state(window); // <-- add this
    } else {
      window->DC.CursorPos.x = layout.StartPos.x;
      reset_line_state(window); // <-- add this
    }

    layout.CurrentItemIndex++;
  }

  // Create spring item
  ImGuiLayoutItem *spring_item = gen_item(layout, ImGuiLayoutItemType_Spring);
  spring_item->MeasuredBounds.Min = spring_item->MeasuredBounds.Max =
      window->DC.CursorPos;
  spring_item->SpringWeight = ImMax(0.0f, weight);

  if (spacing < 0.0f) {
    const ImVec2 is = ImGui::GetStyle().ItemSpacing;
    spacing = (layout.Type == ImGuiLayoutType_Horizontal) ? is.x : is.y;
  }
  spring_item->SpringSpacing = spacing;

  // Visualize spacing+spring so later items position correctly for
  // this frame
  if (spring_item->SpringSize > 0.0f || spacing > 0.0f) {
    ImVec2 spring_size, spring_spacing;
    if (layout.Type == ImGuiLayoutType_Horizontal) {
      spring_spacing = ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y);
      spring_size =
          ImVec2(spacing + spring_item->SpringSize, layout.CurrentSize.y);
    } else {
      spring_spacing = ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f);
      spring_size =
          ImVec2(layout.CurrentSize.x, spacing + spring_item->SpringSize);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImFloor(spring_spacing));
    ImGui::Dummy(ImFloor(spring_size));
    ImGui::PopStyleVar();
  }

  if (layout.Type == ImGuiLayoutType_Horizontal) {
    window->DC.CursorPos.y = layout.StartPos.y;
  } else {
    window->DC.CursorPos.x = layout.StartPos.x;
  }
  reset_line_state(window);

  layout.CurrentItemIndex++;

  // Start next item
  {
    ImGuiLayoutItem &item = *gen_item(layout, ImGuiLayoutItemType_Item);
    item.CurrentAlign = (layout.Align < 0.0f) ? 0.5f : layout.Align;
    item.CurrentAlign = ImClamp(item.CurrentAlign, 0.0f, 1.0f);
    item.CurrentAlignOffset = 0.0f;

    if (item.CurrentAlign > 0.0f) {
      const float off = item.CurrentAlignOffset =
          calc_item_align_offset(layout, item);
      if (layout.Type == ImGuiLayoutType_Horizontal)
        window->DC.CursorPos.y += off;
      else {
        signed_indent(off);
        window->DC.CursorPos.x += off;
      }
    }

    item.MeasuredBounds.Min = item.MeasuredBounds.Max = window->DC.CursorPos;
    item.VertexIndexBegin = ImDrawList_GetCurrentVtxIndex(window->DrawList);
    item.VertexIndexEnd = item.VertexIndexBegin;
  }
}
} // namespace ImGui

// --------------------------------------------------------
// Internal API
// --------------------------------------------------------
ImGuiLayoutType ImGuiInternal::GetCurrentLayoutType(ImGuiID window_id) {
  auto *ws = get_window_layout_state(window_id);
  ImGuiLayoutType lt = ws->Window->DC.LayoutType;
  if (ws->CurrentLayout)
    lt = ws->CurrentLayout->Type;
  return lt;
}

void ImGuiInternal::UpdateItemRect(ImGuiID window_id, const ImVec2 &min,
                                   const ImVec2 &max) {
  auto *ws = get_window_layout_state(window_id);
  if (ws->CurrentLayoutItem)
    ws->CurrentLayoutItem->MeasuredBounds.Max =
        ImMax(ws->CurrentLayoutItem->MeasuredBounds.Max, max);
  (void)min;
}

// --------------------------------------------------------
// Public API
// --------------------------------------------------------
void ImGui::BeginHorizontal(const char *str_id, const ImVec2 &size,
                            float align) {
  ImGuiWindow *w = GetCurrentWindow();
  begin_layout(w->GetID(str_id), ImGuiLayoutType_Horizontal, size, align);
}
void ImGui::BeginHorizontal(const void *ptr_id, const ImVec2 &size,
                            float align) {
  ImGuiWindow *w = GetCurrentWindow();
  begin_layout(w->GetID(ptr_id), ImGuiLayoutType_Horizontal, size, align);
}
void ImGui::BeginHorizontal(int id, const ImVec2 &size, float align) {
  ImGuiWindow *w = GetCurrentWindow();
  begin_layout(w->GetID((void *)(intptr_t)id), ImGuiLayoutType_Horizontal, size,
               align);
}
void ImGui::EndHorizontal() { end_layout(ImGuiLayoutType_Horizontal); }

void ImGui::BeginVertical(const char *str_id, const ImVec2 &size, float align) {
  ImGuiWindow *w = GetCurrentWindow();
  begin_layout(w->GetID(str_id), ImGuiLayoutType_Vertical, size, align);
}
void ImGui::BeginVertical(const void *ptr_id, const ImVec2 &size, float align) {
  ImGuiWindow *w = GetCurrentWindow();
  begin_layout(w->GetID(ptr_id), ImGuiLayoutType_Vertical, size, align);
}
void ImGui::BeginVertical(int id, const ImVec2 &size, float align) {
  ImGuiWindow *w = GetCurrentWindow();
  begin_layout(w->GetID((void *)(intptr_t)id), ImGuiLayoutType_Vertical, size,
               align);
}
void ImGui::EndVertical() { end_layout(ImGuiLayoutType_Vertical); }

void ImGui::Spring(float weight, float spacing) {
  auto *ws = get_current_window_layout_state();
  IM_ASSERT(ws && ws->CurrentLayout && "Spring() must be inside a layout");
  add_spring(*ws->CurrentLayout, weight, spacing);
}

void ImGui::SuspendLayout() {
  ImGui::PushID((void *)0xFEEDBEEF); /* marker */
  ImGui::Dummy(ImVec2(0, 0));
  ImGui::PopID();
  ImGui::GetCurrentWindow();
  ImGui::GetCurrentWindow();
  ImGui::GetCurrentWindow(); /* no-op but keeps API */
}
void ImGui::ResumeLayout() { /* no-op in this implementation; kept for API
                                compatibility */
}

#endif // IMGUI_DISABLE
