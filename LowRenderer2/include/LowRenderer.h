#pragma once

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

    MaterialTypes &get_material_types();

    void initialize();
    void cleanup();
    void prepare_tick(float p_Delta);
    void tick(float p_Delta);

    void check_window_resize(float p_Delta);

    void load_mesh(Mesh p_Mesh);
    Texture load_texture(Util::String p_ImagePath);

    Texture get_default_texture();
    Material get_default_material();
    Material get_default_material_texture();

    void
    submit_thumbnail_creation(ThumbnailCreationSchedule p_Schedule);
  } // namespace Renderer
} // namespace Low
