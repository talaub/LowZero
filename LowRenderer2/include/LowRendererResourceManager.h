#pragma once

#include "LowRendererMesh.h"
#include "LowRendererFont.h"
#include "LowRendererEditorImage.h"

namespace Low {
  namespace Renderer {
    namespace ResourceManager {
      bool load_mesh(Mesh p_Mesh);
      bool upload_mesh(Mesh p_Mesh);
      // bool unload_mesh_resource(MeshResource p_MeshResource);

      bool load_texture(Texture p_Texture);
      bool load_font(Font p_Font);
      bool load_editor_image(EditorImage p_EditorImage);

      void tick(float p_Delta);

      bool parse_mesh_resource_config(Util::String p_Path,
                                      Util::Yaml::Node &p_Node,
                                      MeshResourceConfig &p_Config);
    } // namespace ResourceManager
  } // namespace Renderer
} // namespace Low
