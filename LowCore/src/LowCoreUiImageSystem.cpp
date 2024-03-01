#include "LowCoreUiImageSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreUiImage.h"
#include "LowCoreUiDisplay.h"
#include "LowCoreTaskScheduler.h"
#include "LowCoreDebugGeometry.h"

#include <cmath>
#include "microprofile.h"
#include <stdint.h>

namespace Low {
  namespace Core {
    namespace UI {
      namespace System {
        namespace Image {
          Renderer::Mesh g_Mesh;

          static void create_mesh()
          {
            Util::Resource::MeshInfo l_MeshInfo;
            l_MeshInfo.indices = {0, 1, 2, 2, 3, 0};

#define NORMAL_Z_MULT 1.0f

#if 0
            {
              Util::Resource::Vertex l_Vertex;
              l_Vertex.position = {-0.5f, -0.5f, 0.0f};
              l_Vertex.texture_coordinates = {0.0f, 1.0f};
              l_Vertex.normal = {0.0f, 0.0f, 1.0f * NORMAL_Z_MULT};

              l_MeshInfo.vertices.push_back(l_Vertex);
            }
            {
              Util::Resource::Vertex l_Vertex;
              l_Vertex.position = {0.5f, -0.5f, 0.0f};
              l_Vertex.texture_coordinates = {1.0f, 1.0f};
              l_Vertex.normal = {0.0f, 0.0f, 1.0f * NORMAL_Z_MULT};

              l_MeshInfo.vertices.push_back(l_Vertex);
            }
            {
              Util::Resource::Vertex l_Vertex;
              l_Vertex.position = {0.5f, 0.5f, 0.0f};
              l_Vertex.texture_coordinates = {1.0f, 0.0f};
              l_Vertex.normal = {0.0f, 0.0f, 1.0f * NORMAL_Z_MULT};

              l_MeshInfo.vertices.push_back(l_Vertex);
            }
            {
              Util::Resource::Vertex l_Vertex;
              l_Vertex.position = {-0.5f, 0.5f, 0.0f};
              l_Vertex.texture_coordinates = {0.0f, 0.0f};
              l_Vertex.normal = {0.0f, 0.0f, 1.0f * NORMAL_Z_MULT};

              l_MeshInfo.vertices.push_back(l_Vertex);
            }
#else
            {
              Util::Resource::Vertex l_Vertex;
              l_Vertex.position = {0.0f, 0.0f, 0.0f};
              l_Vertex.texture_coordinates = {0.0f, 1.0f};
              l_Vertex.normal = {0.0f, 0.0f, 1.0f * NORMAL_Z_MULT};

              l_MeshInfo.vertices.push_back(l_Vertex);
            }
            {
              Util::Resource::Vertex l_Vertex;
              l_Vertex.position = {1.0f, 0.0f, 0.0f};
              l_Vertex.texture_coordinates = {1.0f, 1.0f};
              l_Vertex.normal = {0.0f, 0.0f, 1.0f * NORMAL_Z_MULT};

              l_MeshInfo.vertices.push_back(l_Vertex);
            }
            {
              Util::Resource::Vertex l_Vertex;
              l_Vertex.position = {1.0f, 1.0f, 0.0f};
              l_Vertex.texture_coordinates = {1.0f, 0.0f};
              l_Vertex.normal = {0.0f, 0.0f, 1.0f * NORMAL_Z_MULT};

              l_MeshInfo.vertices.push_back(l_Vertex);
            }
            {
              Util::Resource::Vertex l_Vertex;
              l_Vertex.position = {0.0f, 1.0f, 0.0f};
              l_Vertex.texture_coordinates = {0.0f, 0.0f};
              l_Vertex.normal = {0.0f, 0.0f, 1.0f * NORMAL_Z_MULT};

              l_MeshInfo.vertices.push_back(l_Vertex);
            }
#endif

            g_Mesh =
                Renderer::upload_mesh(N(UI_FLAT_MESH), l_MeshInfo);

#undef NORMAL_Z_MULT
          }

          void tick(float p_Delta, Util::EngineState p_State)
          {
            if (p_State != Util::EngineState::PLAYING) {
              return;
            }

            MICROPROFILE_SCOPEI("Core", "UiImageSystem::TICK",
                                MP_RED);

            Component::Image *l_Images =
                Component::Image::living_instances();

            for (uint32_t i = 0u;
                 i < Component::Image::living_count(); ++i) {
              Component::Image i_Image = l_Images[i];

              Component::Display i_Display =
                  i_Image.get_element().get_display();

              if (!i_Image.get_texture().is_alive()) {
                continue;
              }

              if (!i_Image.get_texture().is_loaded()) {
                continue;
              }

              Renderer::RenderObject i_RenderObject;
              i_RenderObject.mesh = get_mesh();
              i_RenderObject.material =
                  i_Image.get_renderer_material();
              i_RenderObject.transform = i_Display.get_world_matrix();
              i_RenderObject.useSkinningBuffer = false;
              i_RenderObject.vertexBufferStartOverride = 0;
              i_RenderObject.entity_id =
                  i_Image.get_element().get_index();
              i_RenderObject.clickPassthrough =
                  i_Image.get_element().is_click_passthrough();

              Renderer::get_main_renderflow().register_renderobject(
                  i_RenderObject);
            }
          }

          Renderer::Mesh get_mesh()
          {
            if (!g_Mesh.is_alive()) {
              create_mesh();
            }

            return g_Mesh;
          }
        } // namespace Image
      }   // namespace System
    }     // namespace UI
  }       // namespace Core
} // namespace Low
