#include "LowCoreUiImageSystem.h"

#include "LowMath.h"
#include "LowRenderer.h"
#include "LowRendererPrimitives.h"
#include "LowRendererUiCanvas.h"
#include "LowRendererUiRenderObject.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreUiImage.h"
#include "LowCoreUiDisplay.h"
#include "LowCoreTaskScheduler.h"
#include "LowCoreDebugGeometry.h"

#include <cmath>
#include <stdint.h>

namespace Low {
  namespace Core {
    namespace UI {
      namespace System {
        namespace Image {
          void tick(float p_Delta, Util::EngineState p_State)
          {
            Component::Image *l_Images =
                Component::Image::living_instances();

            for (uint32_t i = 0u;
                 i < Component::Image::living_count(); ++i) {
              Component::Image i_Image = l_Images[i];

              if (i_Image.get_element().get_view().is_alive() &&
                  i_Image.get_element()
                      .get_view()
                      .is_view_template()) {
                continue;
              }

              Component::Display i_Display =
                  i_Image.get_element().get_display();

              if (i_Image.get_render_object().is_alive() &&
                  i_Image.is_dirty()) {
                i_Image.get_render_object().destroy();
              }

              i_Image.set_dirty(false);

              bool i_NewCreated = false;

              if (i_Image.get_material().is_alive() &&
                  i_Image.get_texture().is_alive() &&
                  !i_Image.get_render_object().is_alive()) {

                Renderer::UiCanvas i_Canvas =
                    i_Image.get_element().get_canvas();

                Renderer::UiRenderObject i_RenderObject =
                    Renderer::UiRenderObject::make(
                        i_Canvas,
                        Renderer::get_primitives().unitQuad);

                i_RenderObject.set_material(i_Image.get_material());
                i_RenderObject.set_texture(i_Image.get_texture());

                i_Image.set_render_object(i_RenderObject);

                i_NewCreated = true;
              }

              if (i_Image.get_render_object().is_alive()) {
                Math::Vector3 i_Position(0, 0, 0);
                i_Position.x =
                    i_Display.get_absolute_pixel_position().x;
                i_Position.y =
                    i_Display.get_absolute_pixel_position().y;

                const Math::Vector2 i_Size =
                    i_Display.get_absolute_pixel_scale();

                i_Image.get_render_object().set_position(i_Position);
                i_Image.get_render_object().set_rotation2D(
                    i_Display.get_absolute_rotation());
                i_Image.get_render_object().set_size(
                    i_Display.get_absolute_pixel_scale());
                i_Image.get_render_object().set_z_sorting(
                    i_Display.get_absolute_layer());
              }
            }
          }
        } // namespace Image
      } // namespace System
    } // namespace UI
  } // namespace Core
} // namespace Low
