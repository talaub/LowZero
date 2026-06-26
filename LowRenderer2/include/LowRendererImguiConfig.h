#pragma once

#include "LowRenderer2Api.h"

#ifdef IMGUI_API
#undef IMGUI_API
#endif
#define IMGUI_API LOW_RENDERER2_API
