#pragma once

#include "LowUtil.h"

#include "LowRendererMesh.h"
#include "LowRendererTexture.h"
#include "LowRendererMaterialType.h"

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

    MaterialTypes &get_material_types();

    void initialize();
    void cleanup();
    void prepare_tick(float p_Delta);
    void tick(float p_Delta);

    void check_window_resize(float p_Delta);

    void load_mesh(Mesh p_Mesh);
    Texture load_texture(Util::String p_ImagePath);

    Texture get_default_texture();
  } // namespace Renderer
} // namespace Low
