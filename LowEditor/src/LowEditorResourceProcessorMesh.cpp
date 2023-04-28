#include "LowEditorResourceProcessorMesh.h"

#include <gli/gli.hpp>
#include <gli/make_texture.hpp>
#include <gli/save_ktx.hpp>
#include <gli/generate_mipmaps.hpp>

#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>

#include "LowUtilAssert.h"

namespace Low {
  namespace Editor {
    namespace ResourceProcessor {

      namespace Mesh {
        void process(Util::String p_FilePath, Util::String p_OutputPath)
        {
          Assimp::Importer l_Importer;

          const aiScene *l_AiScene = l_Importer.ReadFile(p_FilePath.c_str(), 0);

          LOW_ASSERT(l_AiScene, "Could not load mesh scene from file");

          Assimp::Exporter l_Exporter;
          l_Exporter.Export(l_AiScene, "glb2", p_OutputPath.c_str(),
                            aiProcess_Triangulate |
                                aiProcess_FixInfacingNormals |
                                aiProcess_ConvertToLeftHanded);
        }
      } // namespace Mesh
    }   // namespace ResourceProcessor
  }     // namespace Editor
} // namespace Low
