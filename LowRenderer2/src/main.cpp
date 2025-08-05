#include "LowUtilLogger.h"
#include "LowUtil.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include "LowRendererCompatibility.h"

#include "LowRenderer.h"
#include "LowRendererRenderObject.h"
#include "LowRendererRenderView.h"
#include "LowRendererRenderScene.h"
#include "LowRendererMaterial.h"
#include "LowRendererMaterialType.h"
#include "LowRendererRenderStep.h"
#include "LowRendererPointLight.h"
#include "LowRendererMesh.h"

#include "imgui_impl_sdl2.h"

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment,
                     size_t alignmentOffset, const char *pName,
                     int flags, unsigned debugFlags, const char *file,
                     int line)
{
  return malloc(size);
}

Low::Util::Window g_MainWindow;

Low::Renderer::RenderView g_RenderView;
Low::Renderer::RenderScene g_RenderScene;

Low::Renderer::RenderObject g_RenderObject;

Low::Renderer::MaterialType g_SolidBaseMaterialType;
Low::Renderer::Material g_TestMaterial;

Low::Math::Vector3 g_Position(0.0f);
Low::Math::Quaternion g_Rotation(0.0f, 0.0f, 1.0f, 0.0f);
Low::Math::Vector3 g_Scale(1.0f);

Low::Renderer::PointLight g_PointLight;

namespace Low {
  namespace Util {
    Window &Window::get_main_window()
    {
      return g_MainWindow;
    }
  } // namespace Util
} // namespace Low
  //
Low::Math::UVector2 g_Dimensions{1700, 900};

void draw()
{
  Low::Renderer::prepare_tick(0.1f);

  ImGui::Begin("Cube");
  if (ImGui::DragFloat3("Position", (float *)&g_Position)) {
    Low::Math::Matrix4x4 l_LocalMatrix(1.0f);

    l_LocalMatrix = glm::translate(l_LocalMatrix, g_Position);
    l_LocalMatrix *= glm::toMat4(g_Rotation);
    l_LocalMatrix = glm::scale(l_LocalMatrix, g_Scale);

    g_RenderObject.set_world_transform(l_LocalMatrix);
  }
  ImGui::End();

  {
    ImGui::Begin("Light");
    Low::Math::Vector3 pos = g_PointLight.get_world_position();
    if (ImGui::DragFloat3("Position", (float *)&pos)) {
      g_PointLight.set_world_position(pos);
    }

    float intensity = g_PointLight.get_intensity();
    if (ImGui::DragFloat("Intensity", &intensity)) {
      g_PointLight.set_intensity(intensity);
    }

    float range = g_PointLight.get_range();
    if (ImGui::DragFloat("Range", &range)) {
      g_PointLight.set_range(range);
    }

    ImGui::End();
  }
  Low::Renderer::tick(0.1f);
}

bool stop_rendering = false;

void run()
{
  SDL_Event e;
  bool bQuit = false;

  // main loop
  while (!bQuit) {
    // Handle events on queue
    while (SDL_PollEvent(&e) != 0) {
      // close the window when user alt-f4s or clicks the X button
      if (e.type == SDL_QUIT)
        bQuit = true;

      if (e.type == SDL_WINDOWEVENT) {
        if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
          stop_rendering = true;
        }
        if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
          stop_rendering = false;
        }
      }

      ImGui_ImplSDL2_ProcessEvent(&e);
    }

    // do not draw if we are minimized
    if (stop_rendering) {
      // throttle the speed to avoid the endless spinning
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    Low::Renderer::check_window_resize(0.0f);

    draw();
  }
}

void init()
{
  Low::Renderer::initialize();
  {
    Low::Util::String l_BasePath = Low::Util::get_project().dataPath;
    l_BasePath += "/_internal/assets/meshes/spaceship.glb";

    Low::Renderer::Mesh l_Mesh = Low::Renderer::load_mesh(l_BasePath);

    Low::Math::Matrix4x4 l_LocalMatrix(1.0f);

    l_LocalMatrix = glm::translate(l_LocalMatrix, g_Position);
    l_LocalMatrix *= glm::toMat4(g_Rotation);
    l_LocalMatrix = glm::scale(l_LocalMatrix, g_Scale);

    g_RenderScene = Low::Renderer::RenderScene::make("Default");

    {
      g_PointLight = Low::Renderer::PointLight::make(g_RenderScene);
      g_PointLight.set_world_position(0.0f, 5.0f, 0.0f);
      g_PointLight.set_range(5.0f);
      g_PointLight.set_intensity(1.0f);
      Low::Math::ColorRGB l_Color(1.0f, 1.0f, 1.0f);
      g_PointLight.set_color(l_Color);
    }

    g_RenderView = Low::Renderer::RenderView::make("Default");
    g_RenderView.set_dimensions(g_Dimensions);
    g_RenderView.set_render_scene(g_RenderScene);

    g_RenderView.add_step(Low::Renderer::RenderStep::find_by_name(
        RENDERSTEP_SOLID_MATERIAL_NAME));
    g_RenderView.add_step(Low::Renderer::RenderStep::find_by_name(
        RENDERSTEP_LIGHTCULLING_NAME));
    g_RenderView.add_step(Low::Renderer::RenderStep::find_by_name(
        RENDERSTEP_SSAO_NAME));
    g_RenderView.add_step(Low::Renderer::RenderStep::find_by_name(
        RENDERSTEP_LIGHTING_NAME));

    g_SolidBaseMaterialType =
        Low::Renderer::MaterialType::make(N(solid_base));
    g_SolidBaseMaterialType.add_input(
        N(base_color), Low::Renderer::MaterialTypeInputType::VECTOR3);
    g_SolidBaseMaterialType.finalize();

    g_SolidBaseMaterialType.set_draw_vertex_shader_path(
        "solid_base.vert");
    g_SolidBaseMaterialType.set_draw_fragment_shader_path(
        "solid_base.frag");

    g_TestMaterial = Low::Renderer::Material::make(
        N(TestMaterial), g_SolidBaseMaterialType);
    g_TestMaterial.set_property_vector3(
        N(base_color), Low::Math::Vector3(1.0f, 0.0f, 0.0f));

    Low::Math::Vector3 p(0.0f, 0.0f, -14.0f);
    Low::Math::Vector3 d(0.0f, 0.0f, 1.0f);

    g_RenderView.set_camera_position(p);
    g_RenderView.set_camera_direction(d);

    g_RenderObject =
        Low::Renderer::RenderObject::make(g_RenderScene, l_Mesh);
    g_RenderObject.set_material(g_TestMaterial);
    g_RenderObject.set_world_transform(l_LocalMatrix);
  }
}

void cleanup()
{
  Low::Renderer::cleanup();
}

int main(int argc, char *argv[])
{
  Low::Util::initialize();

  SDL_Init(SDL_INIT_VIDEO);

  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

  g_MainWindow.sdlwindow =
      SDL_CreateWindow("LowRenderer2", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, g_Dimensions.x,
                       g_Dimensions.y, window_flags);

  init();
  LOW_LOG_DEBUG << "Test" << LOW_LOG_END;

  run();

  cleanup();

  LOW_LOG_DEBUG << "Testrenderer2" << LOW_LOG_END;

  SDL_DestroyWindow(g_MainWindow.sdlwindow);

  Low::Util::cleanup();
  return 0;
}
