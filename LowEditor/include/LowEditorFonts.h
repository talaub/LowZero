#pragma once

#include "LowEditorApi.h"

#include <imgui.h>
#include "LowUtilContainers.h"

namespace Low {
  namespace Editor {
    namespace Fonts {

      enum class Family
      {
        Roboto
      };
      enum class Weight
      {
        Regular,
        Medium,
        Bold,
        Light
      };

      // Describe a font you want.
      struct Spec
      {
        Family family;
        Weight weight;
        float size_px; // request in logical (pre-DPI) pixels
        // Optional: stylistic tweaks per spec if you like
        bool operator==(const Spec &o) const
        {
          return family == o.family && weight == o.weight &&
                 size_px == o.size_px;
        }
      };

      // Hash for Spec (so it can be a key)
      struct SpecHash
      {
        size_t operator()(const Spec &s) const
        {
          size_t h = 1469598103934665603ull;
          auto mix = [&](uint64_t v) {
            h ^= v;
            h *= 1099511628211ull;
          };
          mix((uint64_t)s.family);
          mix((uint64_t)s.weight);
          mix(*(uint32_t *)&s.size_px);
          return h;
        }
      };

      void initialize(float base_dpi_scale = 1.0f);
      void rebuild(); // call if DPI or theme changes
      LOW_EDITOR_API ImFont *get(const Spec &spec);

      // Optional convenience
      inline ImFont *UI(float size_px = 19.0f,
                        Weight w = Weight::Regular)
      {
        return get({Family::Roboto, w, size_px});
      }

      // Let callers choose which sizes you bake up-front
      void set_preset_sizes(const Util::List<float> &sizes);

      // Call once per frame if you support multi-viewport DPI changes
      void tick_dpi(float new_dpi_scale);

      // Config
      struct Paths
      {
        Util::String roboto_regular_ttf;
        Util::String roboto_medium_ttf;
        Util::String roboto_bold_ttf;
        Util::String roboto_light_ttf;

        Util::String codicons_ttf; // IconFontCppHeaders: Codicons
        Util::String lucide_ttf;   // If you have a Lucide TTF
      };
      void set_paths(const Paths &);
    } // namespace Fonts
  } // namespace Editor
} // namespace Low
