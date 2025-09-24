#pragma once

#include "LowRenderer2Api.h"

#include "LowRendererMesh.h"
#include "LowRendererMaterial.h"
#include "LowRendererFont.h"
#include "LowRendererEditorImage.h"

namespace Low {
  namespace Renderer {
    namespace ResourceManager {
      bool LOW_RENDERER2_API load_mesh(Mesh p_Mesh);
      bool upload_mesh(Mesh p_Mesh);

      bool LOW_RENDERER2_API load_texture(Texture p_Texture);
      bool LOW_RENDERER2_API load_font(Font p_Font);
      bool LOW_RENDERER2_API
      load_editor_image(EditorImage p_EditorImage);
      bool LOW_RENDERER2_API load_material(Material p_Material);

      void tick(float p_Delta);

      bool parse_mesh_resource_config(Util::String p_Path,
                                      Util::Yaml::Node &p_Node,
                                      MeshResourceConfig &p_Config);
      bool
      parse_texture_resource_config(Util::String p_Path,
                                    Util::Yaml::Node &p_Node,
                                    TextureResourceConfig &p_Config);

      void register_asset_id(const u64 p_AssetId,
                             const u64 p_AssetHandleId);

      u64 LOW_RENDERER2_API find_asset_by_id(const u64 p_AssetId);

      template <typename T> T find_asset(const u64 p_AssetId)
      {
        return find_asset_by_id(p_AssetId);
      }

      template <typename T>
      void register_asset(const u64 p_AssetId, T p_Asset)
      {
        register_asset_id(p_AssetId, p_Asset.get_id());
      }

    } // namespace ResourceManager
  } // namespace Renderer
} // namespace Low
