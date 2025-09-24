#include "LowEditorFonts.h"
#include <imgui.h>
#include <algorithm>
#include <cassert>

// If you use IconFontCppHeaders, include their headers:
#include "IconsCodicons.h"
// If IconFontCppHeaders provides Lucide: #include "IconsLucide.h"
// Otherwise we’ll build ranges with GlyphRangesBuilder.

namespace Low {
  namespace Editor {
    // --- Internal state ---
    namespace Fonts {
      static Paths g_paths;
      static Util::List<float> g_preset_sizes = {12.f, 14.f, 16.f,
                                                 18.f, 20.f};
      static float g_dpi = 1.0f;

      struct Key
      {
        Family family;
        Weight weight;
        float baked_px; // size actually baked (after DPI)
        bool operator==(const Key &o) const
        {
          return family == o.family && weight == o.weight &&
                 baked_px == o.baked_px;
        }
      };
      struct KeyHash
      {
        size_t operator()(const Key &k) const
        {
          size_t h = 1469598103934665603ull;
          auto mix = [&](uint64_t v) {
            h ^= v;
            h *= 1099511628211ull;
          };
          mix((uint64_t)k.family);
          mix((uint64_t)k.weight);
          mix(*(uint32_t *)&k.baked_px);
          return h;
        }
      };

      static std::unordered_map<Key, ImFont *, KeyHash> g_fonts;

      // --- Helpers ---
      static const char *PathFor(Family fam, Weight w)
      {
        switch (fam) {
        case Family::Roboto:
          switch (w) {
          case Weight::Regular:
            return g_paths.roboto_regular_ttf.c_str();
          case Weight::Medium:
            return g_paths.roboto_medium_ttf.c_str();
          case Weight::Bold:
            return g_paths.roboto_bold_ttf.c_str();
          case Weight::Light:
            return g_paths.roboto_light_ttf.c_str();
          }
          break;
        }
        return nullptr;
      }

      static void AddIconsToAtlas(ImFontAtlas *atlas, float size_px)
      {
        // Merge config for icons
        ImFontConfig cfg = {};
        cfg.MergeMode = true;
        cfg.PixelSnapH = true;
        cfg.OversampleH = 2; // small icon glyphs rasterize cleanly
                             // with modest oversampling
        cfg.OversampleV = 1;
        cfg.GlyphMinAdvanceX = 0.0f; // set >0 for monospaced icons

        cfg.GlyphOffset.y = 3.0f;
        // --- Codicons range ---
        // IconFontCppHeaders usually defines ICON_MIN_CI /
        // ICON_MAX_CI for Codicons.
        static const ImWchar codicons_ranges[] = {ICON_MIN_CI,
                                                  ICON_MAX_CI, 0};
        if (!g_paths.codicons_ttf.empty())
          atlas->AddFontFromFileTTF(g_paths.codicons_ttf.c_str(),
                                    size_px, &cfg, codicons_ranges);

        // --- Lucide range ---
        // If IconFontCppHeaders provides Lucide defines, prefer them:
        // static const ImWchar lucide_ranges[] = { ICON_MIN_LC,
        // ICON_MAX_LC, 0 };
        // atlas->AddFontFromFileTTF(g_paths.lucide_ttf.c_str(),
        // size_px, &cfg, lucide_ranges);

        // If not, you can build a minimal range with only the icons
        // you use: (Example shows how; comment this block out if you
        // have proper MIN/MAX)
        if (!g_paths.lucide_ttf.empty()) {
          ImVector<ImWchar> ranges;
          ImFontGlyphRangesBuilder b;
          // AddExplicitChar for each Lucide codepoint you actually
          // use b.AddChar(0xE000); b.AddChar(0xE001); ... // <- fill
          // from your icon usage Or, if your Lucide TTF uses a
          // contiguous block you know: b.AddRanges(lucide_ranges);
          b.BuildRanges(&ranges);
          atlas->AddFontFromFileTTF(g_paths.lucide_ttf.c_str(),
                                    size_px, &cfg, ranges.Data);
        }
      }

      static float bake_size(float logical_px)
      {
        // Round to a stable pixel size after DPI so caching hits
        // reliably.
        return floorf(logical_px * g_dpi + 0.5f);
      }

      static ImFont *build_one(ImFontAtlas *atlas, Family fam,
                               Weight w, float logical_px)
      {
        float baked_px = bake_size(logical_px);

        // Base font (text)
        ImFontConfig cfg = {};
        cfg.OversampleH =
            3; // good text sharpness without being heavy
        cfg.OversampleV = 2;
        cfg.PixelSnapH = false;
        cfg.RasterizerMultiply =
            1.10f; // gentle weight boost, tweak to taste

        const char *path = PathFor(fam, w);
        IM_ASSERT(path && "Font path missing");

        // Add text font
        ImFont *base =
            atlas->AddFontFromFileTTF(path, baked_px, &cfg);
        IM_ASSERT(base);

        // Merge icon fonts into the *same* size so icons align with
        // text baseline
        AddIconsToAtlas(atlas, baked_px);
        return base;
      }

      static void build_all()
      {
        ImGuiIO &io = ImGui::GetIO();
        ImFontAtlas *atlas = io.Fonts;
        atlas->Clear();

        // Optional atlas flags
        // atlas->Flags |= ImFontAtlasFlags_NoMouseCursors; // if you
        // draw your own atlas->Flags |=
        // ImFontAtlasFlags_NoPowerOfTwoHeight;

        g_fonts.clear();

        const Family families[] = {Family::Roboto};
        const Weight weights[] = {Weight::Regular, Weight::Medium,
                                  Weight::Bold, Weight::Light};

        for (float s : g_preset_sizes) {
          for (auto fam : families) {
            for (auto wt : weights) {
              ImFont *f = build_one(atlas, fam, wt, s);
              Key k{fam, wt, bake_size(s)};
              g_fonts.emplace(k, f);
            }
          }
        }

        // Build atlas now (optional; ImGui will build lazily
        // otherwise)
        unsigned char *pixels;
        int w, h;
        atlas->GetTexDataAsRGBA32(&pixels, &w, &h);
      }

    } // namespace Fonts

    // --- Public API ---
    namespace Fonts {

      void set_paths(const Paths &p)
      {
        g_paths = p;
      }
      void set_preset_sizes(const Util::List<float> &sizes)
      {
        g_preset_sizes = sizes;
      }

      void initialize(float base_dpi_scale)
      {
        g_dpi = (base_dpi_scale > 0.f) ? base_dpi_scale : 1.0f;
        build_all();
      }

      void rebuild()
      {
        build_all();
      }

      void tick_dpi(float new_dpi_scale)
      {
        // If per-monitor DPI changes, you might pick one “UI DPI”
        // here or keep multiple atlases. Simple approach: rebuild
        // when it changes meaningfully.
        if (fabsf(new_dpi_scale - g_dpi) > 0.05f) {
          g_dpi = new_dpi_scale;
          build_all();
        }
      }

      ImFont *get(const Spec &spec)
      {
        Key k{spec.family, spec.weight, bake_size(spec.size_px)};
        auto it = g_fonts.find(k);
        if (it != g_fonts.end())
          return it->second;

        // Not baked yet? Add on-demand (keeps the API flexible)
        ImFontAtlas *atlas = ImGui::GetIO().Fonts;
        ImFont *f =
            build_one(atlas, spec.family, spec.weight, spec.size_px);
        g_fonts.emplace(k, f);
        // If you add on-demand, you may need to call atlas->Build()
        // right now:
        unsigned char *pixels;
        int w, h;
        atlas->GetTexDataAsRGBA32(&pixels, &w, &h);
        return f;
      }

    } // namespace Fonts
  } // namespace Editor
} // namespace Low
