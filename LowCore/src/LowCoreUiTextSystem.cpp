#include "LowCoreUiTextSystem.h"

#include "LowMath.h"
#include "LowRenderer.h"
#include "LowCoreUiElement.h"
#include "LowRendererFont.h"
#include "LowRendererGpuMesh.h"
#include "LowRendererGpuSubmesh.h"
#include "LowRendererPrimitives.h"
#include "LowRendererTextureState.h"
#include "LowRendererUiCanvas.h"
#include "LowRendererUiDrawCommand.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreUiDisplay.h"
#include "LowCoreUiElement.h"
#include "LowCoreUiText.h"
#include "LowCoreTaskScheduler.h"
#include "LowCoreDebugGeometry.h"

#include "LowCoreUiImageSystem.h"
#include <cmath>
#include <stdint.h>

#define RENDER_BOUNDING_BOXES 0

namespace Low {
  namespace Core {
    namespace UI {
      namespace System {
        namespace Text {
          static void clear_text(Component::Text p_Text)
          {
            for (auto i_DrawCommand : p_Text.get_draw_commands()) {
              i_DrawCommand.destroy();
            }
            p_Text.get_draw_commands().clear();
          }

          static void setup_drawcommands(Component::Text p_Text)
          {
            Renderer::Font l_Font = p_Text.get_font();

            if (l_Font.get_texture().get_state() !=
                Renderer::TextureState::LOADED) {
              return;
            }

            UI::Element l_Element = p_Text.get_element();
            Renderer::UiCanvas l_Canvas = l_Element.get_canvas();

            Renderer::Material l_Material =
                Renderer::get_default_material_ui_text();

            Renderer::Mesh l_Mesh =
                Renderer::get_primitives().unitQuad;
            Renderer::GpuMesh l_GpuMesh = l_Mesh.get_gpu();
            Renderer::GpuSubmesh l_GpuSubmesh =
                l_GpuMesh.get_submeshes()[0];

            // TODO: Maybe check if the unit quad is loaded and the
            // gpu mesh is valid and existing

            for (u32 i = 0; i < p_Text.get_text().size(); ++i) {
              const char i_Char = p_Text.get_text()[i];
              Renderer::UiDrawCommand i_DrawCommand =
                  Renderer::UiDrawCommand::make_standalone(
                      l_Canvas, l_GpuSubmesh);
              p_Text.get_draw_commands().push_back(i_DrawCommand);
              i_DrawCommand.set_texture(l_Font.get_texture());
              i_DrawCommand.set_material(l_Material);
              i_DrawCommand.set_color(p_Text.get_color());
              i_DrawCommand.set_uv_rect(
                  l_Font.find_glyph_uvrect(i_Char));
            }
          }

          static void align_text(Component::Text p_Text)
          {
            if (p_Text.get_draw_commands().empty()) {
              return;
            }
            Renderer::Font l_Font = p_Text.get_font();
            Element l_Element = p_Text.get_element();
            Component::Display l_Display = l_Element.get_display();

            const Math::Vector2 l_Position =
                l_Display.get_absolute_pixel_position();

            const Util::String &l_String = p_Text.get_text();
            const float l_TextSize = p_Text.get_size();

            // These are already in the same normalized space as:
            // - glyph bearing
            // - glyph size
            // - glyph advance
            const float l_AscenderNorm =
                static_cast<float>(l_Font.get_ascender());

            const float l_LineHeightNorm =
                static_cast<float>(l_Font.get_line_height());

            // Text position is top-left of the text block.
            // Convert top-of-line to baseline.
            float l_PenX = l_Position.x;
            float l_BaselineY =
                l_Position.y + l_AscenderNorm * l_TextSize;

            for (u32 i = 0; i < l_String.size(); ++i) {
              const char i_Char = l_String[i];

              if (i_Char == '\n') {
                l_PenX = l_Position.x;
                l_BaselineY += l_LineHeightNorm * l_TextSize;
                continue;
              }

              const Renderer::Glyph &i_Glyph =
                  l_Font.find_glyph(i_Char);

              // Space has no visible quad, but still advances the
              // pen.
              if (i_Char == ' ') {
                l_PenX +=
                    static_cast<float>(i_Glyph.advance) * l_TextSize;
                continue;
              }

              Renderer::UiDrawCommand i_DrawCommand =
                  p_Text.get_draw_commands()[i];

              // Top-left of the glyph quad in screen space
              const float i_TopLeftX =
                  l_PenX +
                  static_cast<float>(i_Glyph.bearing.x) * l_TextSize;

              const float i_TopLeftY =
                  l_BaselineY -
                  static_cast<float>(i_Glyph.bearing.y) * l_TextSize;

              const float i_Width =
                  static_cast<float>(i_Glyph.size.x) * l_TextSize;

              const float i_Height =
                  static_cast<float>(i_Glyph.size.y) * l_TextSize;

              // Draw command position is the CENTER of the quad
              const float i_CenterX = i_TopLeftX + i_Width * 0.5f;
              const float i_CenterY = i_TopLeftY + i_Height * 0.5f;

              i_DrawCommand.set_color(p_Text.get_color());

              i_DrawCommand.set_position({i_CenterX, i_CenterY, 0});
              i_DrawCommand.set_size({i_Width, i_Height});

              i_DrawCommand.set_z_sorting(
                  l_Display.get_absolute_layer());

              l_PenX +=
                  static_cast<float>(i_Glyph.advance) * l_TextSize;
            }
          }

          void tick(float p_Delta, Util::EngineState p_State)
          {
            Component::Text *l_Texts =
                Component::Text::living_instances();

            for (uint32_t i = 0u; i < Component::Text::living_count();
                 ++i) {
              Component::Text i_Text = l_Texts[i];
              Element i_Element = i_Text.get_element();
              Component::Display i_Display = i_Element.get_display();

              if (i_Text.get_draw_commands().empty() ||
                  i_Text.is_full_dirty()) {

                if (!i_Text.get_draw_commands().empty()) {
                  clear_text(i_Text);
                }
                setup_drawcommands(i_Text);
                i_Text.set_full_dirty(false);
                i_Text.mark_dirty();
              }

              if (i_Text.is_dirty() || i_Display.is_dirty()) {
                align_text(i_Text);
                i_Text.set_dirty(false);
              }
            }
          }

        } // namespace Text
      } // namespace System
    } // namespace UI
  } // namespace Core
} // namespace Low
