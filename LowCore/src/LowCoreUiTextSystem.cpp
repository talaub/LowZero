#include "LowCoreUiTextSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreUiDisplay.h"
#include "LowCoreUiText.h"
#include "LowCoreTaskScheduler.h"
#include "LowCoreDebugGeometry.h"

#include "LowCoreUiImageSystem.h"
#include <cmath>
#include "microprofile.h"
#include <stdint.h>

#define RENDER_BOUNDING_BOXES 0

namespace Low {
  namespace Core {
    namespace UI {
      namespace System {
        namespace Text {
          Renderer::Material g_Material;

          static Math::Matrix4x4 calculate_character_transform(
              Math::Vector2 p_Position, Math::Vector2 p_Scale,
              float p_Rotation, float p_Layer)
          {
            float l_EffectiveLayer = p_Layer;

#if RENDER_BOUNDING_BOXES
            l_EffectiveLayer += 0.5f;
#endif

            Math::Matrix4x4 l_LocalMatrix(1.0f);

            l_LocalMatrix = glm::translate(
                l_LocalMatrix,
                Math::Vector3(p_Position.x, p_Position.y,
                              l_EffectiveLayer));
            l_LocalMatrix *= glm::toMat4(Math::VectorUtil::from_euler(
                Math::Vector3(0.0f, 0.0f, p_Rotation)));
            l_LocalMatrix =
                glm::scale(l_LocalMatrix,
                           Math::Vector3(p_Scale.x, p_Scale.y, 1.0f));

            return l_LocalMatrix;
          }

          static float
          calculate_word_advance_x(UI::Component::Text p_Text,
                                   u32 p_CurrentIndex)
          {
            float l_Xpos = 0.0f;
            float l_FontSize = p_Text.get_size();
            Core::Font l_Font = p_Text.get_font();
            for (u32 i = p_CurrentIndex; i < p_Text.get_text().size();
                 ++i) {
              char i_Character = p_Text.get_text()[i];
              if (i_Character == ' ') {
                break;
              }
              FontGlyph i_Glyph = l_Font.get_glyphs()[i_Character];

              float i_AdvanceX = (i_Glyph.advance >> 6) * l_FontSize;
              l_Xpos += i_AdvanceX;
            }

            return l_Xpos;
          }

          static float
          calculate_font_size_to_fit(UI::Component::Text p_Text)
          {
            float l_Width = 0.0f;
            Core::Font l_Font = p_Text.get_font();
            UI::Component::Display l_Display =
                p_Text.get_element().get_display();

            for (u32 i = 0; i < p_Text.get_text().size(); ++i) {
              char i_Character = p_Text.get_text()[i];
              FontGlyph i_Glyph = l_Font.get_glyphs()[i_Character];

              float i_AdvanceX = (i_Glyph.advance >> 6);
              l_Width += i_AdvanceX;
            }

            if (l_Width > l_Display.get_absolute_pixel_scale().x) {
              return l_Display.get_absolute_pixel_scale().x / l_Width;
            }

            return p_Text.get_size();
          }

          static void render_text(UI::Component::Text p_Text)
          {

            Component::Display l_Display =
                p_Text.get_element().get_display();

#if RENDER_BOUNDING_BOXES
            {
              Renderer::RenderObject i_RenderObject;
              i_RenderObject.mesh = System::Image::get_mesh();
              i_RenderObject.material = System::Text::get_material();
              i_RenderObject.transform = l_Display.get_world_matrix();
              i_RenderObject.useSkinningBuffer = false;
              i_RenderObject.vertexBufferStartOverride = 0;

              i_RenderObject.color =
                  Math::Color(1.0f, 0.0f, 0.0f, 0.2f);
              i_RenderObject.texture = 0u;

              i_RenderObject.clickPassthrough = true;

              Renderer::get_main_renderflow().register_renderobject(
                  i_RenderObject);
            }
#endif

            Core::Font l_Font = p_Text.get_font();
            float l_Xpos = l_Display.get_absolute_pixel_position().x;
            float l_Ypos = l_Display.get_absolute_pixel_position().y;

            float l_FontSize = p_Text.get_size();
            l_FontSize *=
                p_Text.get_element().get_view().scale_multiplier();
            float l_FontHeight = l_Font.get_font_size();

            char l_LastCharacter = '\0';

            Component::TextContentFitOptions l_ContentFitApproach =
                p_Text.get_content_fit_approach();

            // Content fit
            if (l_ContentFitApproach ==
                Component::TextContentFitOptions::Fit) {
              l_FontSize = calculate_font_size_to_fit(p_Text);
            }

            for (u32 i = 0; i < p_Text.get_text().size(); ++i) {
              char i_Character = p_Text.get_text()[i];

              FontGlyph i_Glyph = l_Font.get_glyphs()[i_Character];

              float i_AdvanceX = (i_Glyph.advance >> 6) * l_FontSize;

              // Word wrap
              if (l_ContentFitApproach ==
                  Component::TextContentFitOptions::WordWrap) {
                if (l_LastCharacter == ' ') {
                  float i_NextWordAdvance =
                      calculate_word_advance_x(p_Text, i);

                  if (i_NextWordAdvance + l_Xpos >
                      (l_Display.get_absolute_pixel_position().x +
                       l_Display.get_absolute_pixel_scale().x)) {

                    l_Xpos =
                        l_Display.get_absolute_pixel_position().x;
                    l_Ypos += (l_FontHeight * 1.5f) * l_FontSize;
                  }
                }
              }

              float i_Xpos =
                  l_Xpos + (i_Glyph.bearing.x * l_FontSize);
              float i_Ypos = l_Ypos + (i_Glyph.size.y * l_FontSize);

              i_Ypos += (l_FontHeight - i_Glyph.size.y +
                         (i_Glyph.size.y - i_Glyph.bearing.y)) *
                        l_FontSize;

              float i_Width = l_FontSize * i_Glyph.size.x;
              float i_Height = l_FontSize * i_Glyph.size.y;

              Renderer::RenderObject i_RenderObject;
              i_RenderObject.mesh = System::Image::get_mesh();
              i_RenderObject.material = System::Text::get_material();
              i_RenderObject.transform =
                  calculate_character_transform(
                      {i_Xpos, i_Ypos}, {i_Width, i_Height},
                      l_Display.get_absolute_rotation(),
                      l_Display.get_absolute_layer_float());
              i_RenderObject.useSkinningBuffer = false;
              i_RenderObject.vertexBufferStartOverride = 0;

              i_RenderObject.color = p_Text.get_color();
              i_RenderObject.texture = p_Text.get_font()
                                           .get_glyphs()[i_Character]
                                           .rendererTexture;

              i_RenderObject.entity_id =
                  p_Text.get_element().get_index();
              i_RenderObject.clickPassthrough =
                  p_Text.get_element().is_click_passthrough();

              Renderer::get_main_renderflow().register_renderobject(
                  i_RenderObject);

              l_Xpos += i_AdvanceX;

              l_LastCharacter = i_Character;
            }
          }

          void tick(float p_Delta, Util::EngineState p_State)
          {
            if (p_State != Util::EngineState::PLAYING) {
              return;
            }

            MICROPROFILE_SCOPEI("Core", "UiTextSystem::TICK", MP_RED);

            Component::Text *l_Texts =
                Component::Text::living_instances();

            for (uint32_t i = 0u; i < Component::Text::living_count();
                 ++i) {
              Component::Text i_Text = l_Texts[i];

              if (i_Text.get_element()
                      .get_view()
                      .is_view_template()) {
                continue;
              }

              render_text(i_Text);
            }
          }

          static void create_material()
          {
            Renderer::MaterialType l_TextMaterialType =
                Renderer::MaterialType::find_by_name(N(ui_text));
            g_Material = Renderer::create_material(
                N(BaseTextMaterial), l_TextMaterialType);
          }

          Renderer::Material get_material()
          {
            if (!g_Material.is_alive()) {
              create_material();
            }

            return g_Material;
          }
        } // namespace Text
      } // namespace System
    } // namespace UI
  } // namespace Core
} // namespace Low
