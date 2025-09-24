#include "LowMath.h"
#include "LowRendererTextureResource.h"
#include "LowRendererTextureState.h"
#include "LowUtilAssert.h"
#include "LowUtilHashing.h"
#include "LowUtilLogger.h"
#include "LowUtil.h"
#include "LowUtilYaml.h"
#include "SDL_mouse.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include "LowRenderer.h"
#include "LowRendererRenderObject.h"
#include "LowRendererRenderView.h"
#include "LowRendererRenderScene.h"
#include "LowRendererMaterial.h"
#include "LowRendererMaterialType.h"
#include "LowRendererRenderStep.h"
#include "LowRendererPointLight.h"
#include "LowRendererMesh.h"
#include "LowRendererResourceImporter.h"
#include "LowRendererPrimitives.h"
#include "LowRendererResourceManager.h"

#include "LowRendererUiCanvas.h"
#include "LowRendererUiRenderObject.h"
#include "LowRendererTextureExport.h"
#include "LowRendererEditorImage.h"

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

Low::Renderer::Font g_Font;
Low::Renderer::UiRenderObject g_UiRenderObject;

Low::Renderer::Material g_TestMaterial;

Low::Renderer::Material g_UiMaterial;

Low::Math::Vector3 g_Position(0.0f);
Low::Math::Quaternion g_Rotation(0.0f, 0.0f, 1.0f, 0.0f);
Low::Math::Vector3 g_Scale(1.0f);

Low::Renderer::PointLight g_PointLight;

Low::Math::UVector2 g_Dimensions{1700, 900};

uint64_t frames = 0;

static void save_material(Low::Renderer::Material p_Material)
{
  using namespace Low;

  if (!p_Material.get_resource().is_alive()) {
    LOW_LOG_ERROR << "Cannot save material to disk that does not "
                     "have a resource associated with it."
                  << LOW_LOG_END;
    return;
  }

  if (!(p_Material.get_state() == Renderer::MaterialState::LOADED ||
        p_Material.get_state() ==
            Renderer::MaterialState::UPLOADINGTOGPU ||
        p_Material.get_state() ==
            Renderer::MaterialState::MEMORYLOADED)) {
    LOW_LOG_ERROR << "Cannot save material to disk that is not "
                     "loaded into memory."
                  << LOW_LOG_END;
    return;
  }

  Renderer::MaterialResource l_Resource = p_Material.get_resource();

  Util::Yaml::Node l_Node;
  p_Material.serialize(l_Node);

  l_Node["version"] = 2;

  Util::Yaml::write_file(l_Resource.get_path().c_str(), l_Node);
}

static void save_material(Low::Renderer::Material p_Material,
                          Low::Util::String p_Path)
{
  using namespace Low;
  if (p_Material.get_resource().is_alive()) {
    if (p_Material.get_resource().get_path() == p_Path) {
      save_material(p_Material);
    } else {
      LOW_LOG_ERROR << "Material already has a resource associated "
                       "but the paths don't match."
                    << LOW_LOG_END;
    }
    return;
  }

  Renderer::MaterialResource l_Resource =
      Renderer::MaterialResource::make(p_Path);

  p_Material.set_resource(l_Resource);

  save_material(p_Material);
}

void draw()
{
  frames++;
  Low::Renderer::prepare_tick(0.1f);

  static bool l_FontInit = false;

#if 0
  if (frames == 120) {
    Low::Renderer::ResourceImporter::import_mesh("E:\\spaceship.obj",
                                                 "spaceship");
  }
#endif

#if 0
  if (frames == 200) {
    using namespace Low;
    using namespace Low::Renderer;
    TextureExport l_Export = TextureExport::make(N(Test));
    l_Export.set_texture(g_RenderView.get_lit_image());
    l_Export.set_path("E:\\screenshot.png");

    LOW_LOG_DEBUG << "EXPORT" << LOW_LOG_END;
  }
#endif

  if (!l_FontInit && g_Font.is_alive() && g_Font.is_fully_loaded()) {
    using namespace Low;

    const char l_Char = 'C';

    Math::Vector4 l_UvRect;
    l_UvRect.x = g_Font.get_glyphs()[l_Char].uvMin.x;
    l_UvRect.y = g_Font.get_glyphs()[l_Char].uvMin.y;
    l_UvRect.z = g_Font.get_glyphs()[l_Char].uvMax.x;
    l_UvRect.w = g_Font.get_glyphs()[l_Char].uvMax.y;

    LOW_LOG_PROFILE << l_UvRect << LOW_LOG_END;

    g_UiRenderObject.set_uv_rect(l_UvRect);

    l_FontInit = true;
  }

  {
    using namespace Low;
    using namespace Low::Renderer;
    if (EditorImage::living_count() > 0) {
      EditorImage l_EditorImage = EditorImage::living_instances()[0];

      if (l_EditorImage.get_state() == TextureState::LOADED &&
          l_EditorImage.get_gpu().is_imgui_texture_initialized()) {
        ImGui::Begin("Editor Image");
        ImGui::Image(l_EditorImage.get_gpu().get_imgui_texture_id(),
                     ImVec2(256, 256));
        ImGui::End();
      } else if (l_EditorImage.get_state() ==
                 TextureState::UNLOADED) {
        ResourceManager::load_editor_image(l_EditorImage);
      }
    }
  }

  ImGui::Begin("Cube");
  if (ImGui::DragFloat3("Position", (float *)&g_Position)) {
    Low::Math::Matrix4x4 l_LocalMatrix(1.0f);

    l_LocalMatrix = glm::translate(l_LocalMatrix, g_Position);
    l_LocalMatrix *= glm::toMat4(g_Rotation);
    l_LocalMatrix = glm::scale(l_LocalMatrix, g_Scale);

    g_RenderObject.set_world_transform(l_LocalMatrix);
  }
  ImGui::End();

  if (Low::Renderer::get_primitives().unitCube.get_state() ==
      Low::Renderer::MeshState::LOADED) {
    using namespace Low;
    using namespace Low::Renderer;
    DebugGeometryDraw l_Draw;
    l_Draw.depthTest = true;
    l_Draw.wireframe = true;
    l_Draw.submesh =
        get_primitives().unitCube.get_gpu().get_submeshes()[0];
    l_Draw.color = Math::Color(1.0f, 0.0f, 0.0f, 0.5f);

    Low::Math::Matrix4x4 l_LocalMatrix(1.0f);

    l_LocalMatrix = glm::translate(l_LocalMatrix, {1.0f, 1.0f, 1.0f});
    l_LocalMatrix *=
        glm::toMat4(Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
    l_LocalMatrix = glm::scale(l_LocalMatrix, {1.0f, 1.0f, 1.0f});

    l_Draw.transform = l_LocalMatrix;

    g_RenderView.add_debug_geometry(l_Draw);
  }

  if (g_PointLight.is_alive()) {
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

  if (false) {
    int x;
    int y;
    SDL_GetMouseState(&x, &y);
    const u32 readback = g_RenderView.read_object_id_px({x, y});

    LOW_LOG_DEBUG << "Readback: " << readback << LOW_LOG_END;
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

    Low::Renderer::Mesh l_Mesh =
        Low::Renderer::Mesh::find_by_name(N(spaceship));

    Low::Math::Matrix4x4 l_LocalMatrix(1.0f);

    l_LocalMatrix = glm::translate(l_LocalMatrix, g_Position);
    l_LocalMatrix *= glm::toMat4(g_Rotation);
    l_LocalMatrix = glm::scale(l_LocalMatrix, g_Scale);

    g_RenderScene = Low::Renderer::RenderScene::make("Default");

    if (true) {
      g_PointLight = Low::Renderer::PointLight::make(g_RenderScene);
      g_PointLight.set_world_position(0.0f, 1.0f, -10.0f);
      g_PointLight.set_range(20.0f);
      g_PointLight.set_intensity(1.0f);
      Low::Math::ColorRGB l_Color(1.0f, 1.0f, 1.0f);
      g_PointLight.set_color(l_Color);
    }

    Low::Renderer::MaterialTypes &l_MaterialTypes =
        Low::Renderer::get_material_types();

    g_RenderView = Low::Renderer::RenderView::make("Default");
    g_RenderView.set_dimensions(g_Dimensions);
    g_RenderView.set_render_scene(g_RenderScene);

    {
      g_RenderScene.set_directional_light_intensity(1.0f);
      g_RenderScene.set_directional_light_color(
          Low::Math::ColorRGB(1.0f, 1.0f, 1.0f));
      g_RenderScene.set_directional_light_direction(
          Low::Math::Vector3(0.0f, -1.0f, 0.5f));
    }

    g_RenderView.add_step(Low::Renderer::RenderStep::find_by_name(
        RENDERSTEP_SOLID_MATERIAL_NAME));
    g_RenderView.add_step(Low::Renderer::RenderStep::find_by_name(
        RENDERSTEP_LIGHTCULLING_NAME));
    g_RenderView.add_step(Low::Renderer::RenderStep::find_by_name(
        RENDERSTEP_SSAO_NAME));
    g_RenderView.add_step(Low::Renderer::RenderStep::find_by_name(
        RENDERSTEP_LIGHTING_NAME));
    g_RenderView.add_step_by_name(RENDERSTEP_DEBUG_GEOMETRY_NAME);
    g_RenderView.add_step(
        Low::Renderer::RenderStep::find_by_name(RENDERSTEP_UI_NAME));
    g_RenderView.add_step_by_name(RENDERSTEP_OBJECT_ID_COPY);

    g_UiMaterial = Low::Renderer::Material::make_gpu_ready(
        N(UiMaterial), l_MaterialTypes.uiText);

#if 0
    g_TestMaterial = Low::Renderer::Material::make(
        N(TestMaterial), l_MaterialTypes.solidBase);
    g_TestMaterial.set_property_vector3(
        N(base_color), Low::Math::Vector3(1.0f, 0.0f, 0.0f));
#else
    g_TestMaterial =
        Low::Renderer::Material::find_by_name(N(TestMaterial));
#endif
#if 0
    g_TestMaterial.set_property_texture(
        N(albedo), Low::Renderer::Texture::find_by_name(N(brick)));
#endif

#if 0
    {
      Low::Util::String l_Path = Low::Util::get_project().dataPath;
      l_Path += "\\TestMaterial.material.yaml";
      save_material(g_TestMaterial, l_Path);
    }
#endif

    {
      using namespace Low::Renderer;
      UiCanvas l_Canvas = UiCanvas::make(N(TestCanvas));
      g_RenderView.add_ui_canvas(l_Canvas);

      g_UiRenderObject = UiRenderObject::make(
          l_Canvas, Low::Renderer::get_primitives().unitQuad);

      g_UiRenderObject.set_position_x(800.0f);
      g_UiRenderObject.set_position_y(400.0f);

      g_UiRenderObject.set_texture(get_default_texture());

      g_UiRenderObject.set_rotation2D(0.0f);
      g_UiRenderObject.set_z_sorting(0);

#if 1
      g_UiRenderObject.set_size(Low::Math::Vector2(150.0f, 150.0f));
      Low::Util::String l_TexturePath =
          Low::Util::get_project().dataPath;
      l_TexturePath += "\\resources\\img2d\\buff.ktx";

      Texture l_Texture = Texture::make(N(TestTexture));
      l_Texture.set_resource(TextureResource::make(l_TexturePath));

      // g_UiRenderObject.set_texture(l_Texture);

      g_Font = Font::living_instances()[0];

      ResourceManager::load_font(g_Font);

      g_UiRenderObject.set_texture(g_Font.get_texture());

      /*
      Texture l_TextureFont = Texture::make(N(TestTexture));
      {
        Low::Util::String l_TexturePath =
            Low::Util::get_project().assetCachePath;
        l_TexturePath += "\\test.msdf.ktx";
        l_TextureFont.set_resource(
            TextureResource::make(l_TexturePath));

        g_UiRenderObject.set_texture(l_TextureFont);
      }
      */

#else
      g_UiRenderObject.set_size(Low::Math::Vector2(50.0f, 50.0f));
#endif

      g_UiRenderObject.set_material(g_UiMaterial);
    }

    Low::Math::Vector3 p(0.0f, 0.0f, -14.0f);
    Low::Math::Vector3 d(0.0f, 0.0f, 1.0f);

    g_RenderView.set_camera_position(p);
    g_RenderView.set_camera_direction(d);

    g_RenderObject =
        Low::Renderer::RenderObject::make(g_RenderScene, l_Mesh);
    g_RenderObject.set_material(g_TestMaterial);
    g_RenderObject.set_world_transform(l_LocalMatrix);
    g_RenderObject.set_object_id(15);
  }
}

void cleanup()
{
  Low::Renderer::cleanup();
}

int main(int argc, char *argv[])
{
  Low::Util::initialize();

  if (false) {
    Low::Renderer::ResourceImporter::import_mesh("E:\\spaceship.obj",
                                                 "spaceship");

    Low::Util::cleanup();
    return 0;
  }
  if (false) {
    Low::Renderer::ResourceImporter::import_font("E:\\roboto.ttf",
                                                 "roboto");
    Low::Util::cleanup();
    return 0;
  }
  if (false) {
    Low::Renderer::ResourceImporter::import_texture("E:\\brick.png",
                                                    "bricks");
    Low::Util::cleanup();
    return 0;
  }

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
