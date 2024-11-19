#include "LowRendererVulkanPipelineManager.h"

#include "LowUtilFileIO.h"
#include "LowUtilProfiler.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace PipelineManager {
        Util::Map<Pipeline, PipelineUtil::GraphicsPipelineBuilder>
            g_GraphicsPipelines;

        Util::Map<Util::String, Pipeline> g_GraphicsSources;

        Util::Map<Util::String, u64> g_SourceTimes;

        Util::Map<Util::String, Util::String> g_SourceOutMapping;

        static bool compile_shader(Util::String p_SourcePath,
                                   Util::String p_OutPath)
        {
          Util::String l_Command =
              "glslc " + p_SourcePath + " -o " + p_OutPath;

          Util::String l_Notice = "Compiling shader " + p_SourcePath;

          LOW_LOG_DEBUG << l_Notice << LOW_LOG_END;
          system(l_Command.c_str());

          return true;
        }

        static bool compile_shader(Util::String p_SourcePath)
        {
          return compile_shader(p_SourcePath,
                                g_SourceOutMapping[p_SourcePath]);
        }

        bool compile_graphics_pipeline(Pipeline p_Pipeline,
                                       bool p_CompileShaders)
        {
          PipelineUtil::GraphicsPipelineBuilder &l_Builder =
              g_GraphicsPipelines[p_Pipeline];

          // Destroy old pipeline
          vkDestroyPipeline(Global::get_device(),
                            p_Pipeline.get_pipeline(), nullptr);

          // #if LOW_RENDERER_COMPILE_SHADERS
          if (p_CompileShaders) {
            compile_shader(l_Builder.vertexShaderPath);
            compile_shader(l_Builder.fragmentShaderPath);
          }
          // #endif

          // Update shaders in builder
          l_Builder.update_shaders();

          VkPipeline l_VkPipeline =
              l_Builder.build_pipeline(Global::get_device());

          // Create new pipeline and assign
          p_Pipeline.set_pipeline(l_VkPipeline);

          return true;
        }

        bool register_graphics_pipeline(
            Pipeline p_Pipeline,
            PipelineUtil::GraphicsPipelineBuilder p_Builder)
        {
          g_GraphicsPipelines[p_Pipeline] = p_Builder;

          g_GraphicsSources[p_Builder.vertexShaderPath] = p_Pipeline;
          g_GraphicsSources[p_Builder.fragmentShaderPath] =
              p_Pipeline;

          g_SourceTimes[p_Builder.vertexShaderPath] =
              Util::FileIO::modified_sync(
                  p_Builder.vertexShaderPath.c_str());
          g_SourceTimes[p_Builder.fragmentShaderPath] =
              Util::FileIO::modified_sync(
                  p_Builder.fragmentShaderPath.c_str());

          g_SourceOutMapping[p_Builder.vertexShaderPath] =
              p_Builder.vertexSpirvPath;
          g_SourceOutMapping[p_Builder.fragmentShaderPath] =
              p_Builder.fragmentSpirvPath;

          compile_graphics_pipeline(p_Pipeline);

          return true;
        }

        bool do_tick(float p_Delta)
        {
          for (auto it = g_SourceTimes.begin();
               it != g_SourceTimes.end(); ++it) {
            Util::String i_SourcePath = it->first;

            uint64_t i_Modified =
                Util::FileIO::modified_sync(i_SourcePath.c_str());

            if (i_Modified == it->second) {
              continue;
            }

            vkDeviceWaitIdle(Global::get_device());

            g_SourceTimes[i_SourcePath] = i_Modified;

            auto i_GraphicsSourcesEntry =
                g_GraphicsSources.find(i_SourcePath);
            if (i_GraphicsSourcesEntry != g_GraphicsSources.end()) {
              Pipeline i_Pipeline = i_GraphicsSourcesEntry->second;

              compile_shader(i_SourcePath);

              compile_graphics_pipeline(i_Pipeline, false);
            }
          }

          return true;
        }

        bool tick(float p_Delta)
        {
          // LOW_PROFILE_CPU("Renderer", "Pipeline manager");
          // #if LOW_RENDERER_COMPILE_SHADERS
          static float l_Count = 100.0f;
          if (l_Count > 1.0f) {
            do_tick(p_Delta);
            l_Count = 0.0f;
          }
          l_Count += p_Delta;
          // #endif

          return true;
        }
      } // namespace PipelineManager
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
