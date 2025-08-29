#pragma once

#include "LowRendererMesh.h"
#include "LowRendererFont.h"

namespace Low {
  namespace Renderer {
    namespace ResourceManager {
      bool load_mesh(Mesh p_Mesh);
      bool upload_mesh(Mesh p_Mesh);
      // bool unload_mesh_resource(MeshResource p_MeshResource);

      bool load_texture(Texture p_Texture);

      bool load_font(Font p_Font);

      void tick(float p_Delta);
    } // namespace ResourceManager
  }   // namespace Renderer
} // namespace Low
