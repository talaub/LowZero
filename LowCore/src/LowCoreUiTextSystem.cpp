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
#include "LowUtilString.h"
#include "glm/trigonometric.hpp"
#include <cmath>
#include <stdint.h>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/constants.hpp>

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

          static bool setup_drawcommands(Component::Text p_Text)
          {
            Renderer::Font l_Font = p_Text.get_font();

            if (!l_Font.get_texture().is_alive() ||
                l_Font.get_texture().get_state() !=
                    Renderer::TextureState::LOADED) {
              return false;
            }

            UI::Element l_Element = p_Text.get_element();
            Renderer::UiCanvas l_Canvas = l_Element.get_canvas();

            if (!l_Canvas.is_alive()) {
              return false;
            }

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

            return true;
          }

          static float
          calculate_glyph_width(Component::Text p_Text,
                                const Renderer::Glyph &p_Glyph)
          {
            const float l_TextSize = p_Text.get_size();

            return static_cast<float>(p_Glyph.advance) * l_TextSize;
          }

          static float calculate_glyph_width(Component::Text p_Text,
                                             const char p_Char)
          {
            Renderer::Font l_Font = p_Text.get_font();

            const Renderer::Glyph &l_Glyph =
                l_Font.find_glyph(p_Char);

            return calculate_glyph_width(p_Text, l_Glyph);
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
            const float l_Rotation =
                l_Display.get_absolute_rotation();
            const float l_RotationRadians = glm::radians(l_Rotation);

            const Math::Vector2 l_Right = glm::rotate(
                Math::Vector2(1.0f, 0.0f), l_RotationRadians);
            const Math::Vector2 l_Down = glm::rotate(
                Math::Vector2(0.0f, 1.0f), l_RotationRadians);

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

              const float i_GlyphWidth =
                  calculate_glyph_width(p_Text, i_Glyph);

              // Space has no visible quad, but still advances the
              // pen.
              if (i_Char == ' ') {
                l_PenX += i_GlyphWidth;
                continue;
              }

              Renderer::UiDrawCommand i_DrawCommand =
                  p_Text.get_draw_commands()[i];

              const float i_Width =
                  static_cast<float>(i_Glyph.size.x) * l_TextSize;

              const float i_Height =
                  static_cast<float>(i_Glyph.size.y) * l_TextSize;

              // Compute the glyph center relative to the text origin
              // in Y-down UI coordinates, then rotate the full
              // offset.
              const float i_DistanceX =
                  l_PenX - l_Position.x +
                  static_cast<float>(i_Glyph.bearing.x) * l_TextSize +
                  i_Width * 0.5f;
              const float i_DistanceY =
                  l_BaselineY - l_Position.y -
                  static_cast<float>(i_Glyph.bearing.y) * l_TextSize +
                  i_Height * 0.5f;
              const Math::Vector2 i_Center = l_Position +
                                             i_DistanceX * l_Right +
                                             i_DistanceY * l_Down;

              i_DrawCommand.set_color(p_Text.get_color());

              i_DrawCommand.set_position({i_Center.x, i_Center.y, 0});
              i_DrawCommand.set_size({i_Width, i_Height});
              i_DrawCommand.set_rotation2D(-l_RotationRadians);

              i_DrawCommand.set_z_sorting(
                  l_Display.get_absolute_layer());

              l_PenX += i_GlyphWidth;
            }
          }

          static void calculate_format(Component::Text p_Text)
          {
            p_Text.get_formatted_text_lines().clear();

            Element l_Element = p_Text.get_element();
            Component::Display l_Display = l_Element.get_display();

            const float l_MaxWidth =
                l_Display.get_absolute_pixel_scale().x;

            const Util::String &l_String = p_Text.get_text();

            Util::StringBuilder l_LineBuilder;
            float l_LineWidth = 0;
            Util::StringBuilder l_WordBuilder;
            float l_WordWidth = 0;

            for (u32 i = 0; i < l_String.size(); ++i) {
              const char i_Char = l_String[i];

              const float i_GlyphWidth =
                  calculate_glyph_width(p_Text, i_Char);

              if (i_Char == ' ') {
                if (l_LineWidth + l_WordWidth > l_MaxWidth) {
                  // Start new line
                  p_Text.get_formatted_text_lines().push_back(
                      l_LineBuilder.get());
                  l_LineBuilder.reset();
                  l_LineWidth = 0;
                }

                // Add word to line
                l_LineBuilder.append(l_WordBuilder.get());
                l_LineWidth += l_WordWidth;
                l_LineBuilder.append(i_Char);
                l_LineWidth += i_GlyphWidth;

                // Reset word builder
                l_WordBuilder.reset();
                l_WordWidth = 0;
              }

              l_WordBuilder.append(i_Char);
              l_WordWidth += i_GlyphWidth;
            }
          }

          enum class RescheduleType
          {
            Full,
            Format,
            Align
          };
          struct Reschedule
          {
            RescheduleType type;
            Component::Text text;
          };

          void tick(float p_Delta, Util::EngineState p_State)
          {
            Util::List<Reschedule> l_Reschedules;

            for (Component::Text i_Text : Component::Text::ms_Dirty) {
              if (!i_Text.is_alive()) {
                continue;
              }
              if (!i_Text.get_font().is_alive()) {
                i_Text.set_font(
                    Renderer::Font::living_instances()[0]);
              }
              Element i_Element = i_Text.get_element();
              Component::Display i_Display = i_Element.get_display();

              if (i_Text.get_draw_commands().empty() ||
                  i_Text.is_full_dirty()) {

                if (!i_Text.get_draw_commands().empty()) {
                  clear_text(i_Text);
                }
                if (!setup_drawcommands(i_Text)) {
                  l_Reschedules.emplace_back(RescheduleType::Full,
                                             i_Text);
                }
                i_Text.set_full_dirty(false);
                i_Text.mark_format_dirty();
              }

              if (i_Text.is_format_dirty()) {
                calculate_format(i_Text);
                i_Text.set_format_dirty(false);
                i_Text.mark_align_dirty();
              }

              if (i_Text.is_align_dirty()) {
                align_text(i_Text);
                i_Text.set_align_dirty(false);
              }
            }

            Component::Text::ms_Dirty.clear();

            for (Reschedule &i_Reschedule : l_Reschedules) {
              if (i_Reschedule.type == RescheduleType::Full) {
                i_Reschedule.text.mark_full_dirty();
              } else if (i_Reschedule.type ==
                         RescheduleType::Format) {
                i_Reschedule.text.mark_format_dirty();
              } else if (i_Reschedule.type == RescheduleType::Align) {
                i_Reschedule.text.mark_align_dirty();
              }
            }
          }

        } // namespace Text
      } // namespace System
    } // namespace UI
  } // namespace Core
} // namespace Low
