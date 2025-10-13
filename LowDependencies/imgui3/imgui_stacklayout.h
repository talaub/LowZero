// dear imgui, v1.86 WIP
// (stack layout headers)

/*

Index of this file:
// [SECTION] Stack Layout API

*/

#pragma once


#ifndef IMGUI_DISABLE
#ifndef IMGUI_API
#define IMGUI_API
#endif

#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui.h>

//-----------------------------------------------------------------------------
// [SECTION] Stack Layout API
//-----------------------------------------------------------------------------

namespace ImGui
{
    // Horizontal
    IMGUI_API void BeginHorizontal(const char* str_id, const ImVec2& size = ImVec2(0, 0), float align = -1.0f);
    IMGUI_API void BeginHorizontal(const void* ptr_id, const ImVec2& size = ImVec2(0, 0), float align = -1.0f);
    IMGUI_API void BeginHorizontal(int id,            const ImVec2& size = ImVec2(0, 0), float align = -1.0f);
    IMGUI_API void EndHorizontal();

    // Vertical
    IMGUI_API void BeginVertical(const char* str_id,  const ImVec2& size = ImVec2(0, 0), float align = -1.0f);
    IMGUI_API void BeginVertical(const void* ptr_id,  const ImVec2& size = ImVec2(0, 0), float align = -1.0f);
    IMGUI_API void BeginVertical(int id,              const ImVec2& size = ImVec2(0, 0), float align = -1.0f);
    IMGUI_API void EndVertical();

    // Helpers
    IMGUI_API void Spring(float weight = 1.0f, float spacing = -1.0f);
    IMGUI_API void SuspendLayout();
    IMGUI_API void ResumeLayout();
}


#endif // #ifndef IMGUI_DISABLE
