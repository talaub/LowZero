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

            Math::Matrix4x4 l_LocalMatrix(1.0f);

            l_LocalMatrix = glm::translate(
                l_LocalMatrix,
                Math::Vector3(p_Position.x, p_Position.y, p_Layer));
            l_LocalMatrix *= glm::toMat4(Math::VectorUtil::from_euler(
                Math::Vector3(0.0f, 0.0f, p_Rotation)));
            l_LocalMatrix =
                glm::scale(l_LocalMatrix,
                           Math::Vector3(p_Scale.x, p_Scale.y, 1.0f));

            return l_LocalMatrix;
          }

          static void render_text(UI::Component::Text p_Text)
          {

            Component::Display l_Display =
                p_Text.get_element().get_display();

            Core::Font l_Font = p_Text.get_font();
            float l_Xpos = l_Display.get_absolute_pixel_position().x;
            float l_Ypos = l_Display.get_absolute_pixel_position().y;

            float l_FontSize = p_Text.get_size();

            for (u32 i = 0; i < p_Text.get_text().size(); ++i) {
              char i_Character = p_Text.get_text()[i];

              FontGlyph i_Glyph = l_Font.get_glyphs()[i_Character];

              float i_Xpos = l_Xpos + i_Glyph.bearing.x * l_FontSize;
              float i_Ypos =
                  l_Ypos +
                  (i_Glyph.size.y - i_Glyph.bearing.y) * l_FontSize;

              float i_Width = l_FontSize * i_Glyph.size.x;
              float i_Height = l_FontSize * i_Glyph.size.y;

              Renderer::RenderObject i_RenderObject;
              i_RenderObject.mesh = System::Image::get_mesh();
              i_RenderObject.material = System::Text::get_material();
              i_RenderObject.transform =
                  calculate_character_transform(
                      {i_Xpos, i_Ypos}, {i_Width, i_Height},
                      l_Display.get_absolute_rotation(), 0.0f);
              i_RenderObject.useSkinningBuffer = false;
              i_RenderObject.vertexBufferStartOverride = 0;

              i_RenderObject.color = p_Text.get_color();
              i_RenderObject.texture = p_Text.get_font()
                                           .get_glyphs()[i_Character]
                                           .rendererTexture;

              Renderer::get_main_renderflow().register_renderobject(
                  i_RenderObject);

              l_Xpos += (i_Glyph.advance >> 6) * l_FontSize;
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
      }   // namespace System
    }     // namespace UI
  }       // namespace Core
} // namespace Low
