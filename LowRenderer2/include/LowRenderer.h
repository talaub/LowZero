#pragma once

#include "LowRenderer2Api.h"

#include "LowUtil.h"

#include "LowRendererMesh.h"
#include "LowRendererTexture.h"
#include "LowRendererMaterialType.h"
#include "LowRendererMaterial.h"
#include "LowRendererRenderScene.h"
#include "LowRendererRenderView.h"
#include "LowRendererTextureExport.h"

namespace Low {
  namespace Renderer {
    struct MaterialTypes
    {
      MaterialType solidBase;
      MaterialType uiBase;
      MaterialType uiText;
      MaterialType debugGeometry;
      MaterialType debugGeometryNoDepth;

      MaterialType debugGeometryWireframe;
      MaterialType debugGeometryNoDepthWireframe;
    };

    enum class ThumbnailCreationState
    {
      SCHEDULED,
      SUBMITTED,
      DONE
    };

    struct ThumbnailCreationSchedule
    {
      RenderScene scene;
      RenderView view;
      RenderObject object;
      Mesh mesh;
      Material material;
      Math::Vector3 viewDirection;
      Util::String path;
      ThumbnailCreationState state;
      TextureExport textureExport;
    };

    LOW_RENDERER2_API MaterialTypes &get_material_types();

    void LOW_RENDERER2_API initialize();
    void LOW_RENDERER2_API cleanup();
    void LOW_RENDERER2_API prepare_tick(float p_Delta);
    void LOW_RENDERER2_API tick(float p_Delta);

    void LOW_RENDERER2_API check_window_resize(float p_Delta);

    void LOW_RENDERER2_API load_mesh(Mesh p_Mesh);
    Texture LOW_RENDERER2_API load_texture(Util::String p_ImagePath);

    Texture LOW_RENDERER2_API get_default_texture();
    Material LOW_RENDERER2_API get_default_material();
    Material LOW_RENDERER2_API get_default_material_texture();

    RenderView LOW_RENDERER2_API get_game_renderview();
    RenderView LOW_RENDERER2_API get_editor_renderview();

    RenderScene LOW_RENDERER2_API get_global_renderscene();

    void LOW_RENDERER2_API
    submit_thumbnail_creation(ThumbnailCreationSchedule p_Schedule);
  } // namespace Renderer
} // namespace Low
