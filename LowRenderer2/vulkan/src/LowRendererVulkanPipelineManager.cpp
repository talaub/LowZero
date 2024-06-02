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

        bool compile_graphics_pipeline(Context &p_Context,
                                       Pipeline p_Pipeline)
        {
          PipelineUtil::GraphicsPipelineBuilder &l_Builder =
              g_GraphicsPipelines[p_Pipeline];

          // Destroy old pipeline
          vkDestroyPipeline(p_Context.device,
                            p_Pipeline.get_pipeline(), nullptr);

          // #if LOW_RENDERER_COMPILE_SHADERS
          {
            Util::String l_Command =
                "glslc " + l_Builder.vertexShaderPath + " -o " +
                l_Builder.vertexSpirvPath;

            Util::String l_Notice =
                "Compiling shader " + l_Builder.vertexShaderPath;

            LOW_LOG_DEBUG << l_Notice << LOW_LOG_END;
            system(l_Command.c_str());
          }
          {
            Util::String l_Command =
                "glslc " + l_Builder.fragmentShaderPath + " -o " +
                l_Builder.fragmentSpirvPath;

            Util::String l_Notice =
                "Compiling shader " + l_Builder.fragmentShaderPath;

            LOW_LOG_DEBUG << l_Notice << LOW_LOG_END;
            system(l_Command.c_str());
          }
          // #endif

          // Update shaders in builder
          l_Builder.update_shaders(p_Context);

          VkPipeline l_VkPipeline =
              l_Builder.build_pipeline(p_Context.device);

          // Create new pipeline and assign
          p_Pipeline.set_pipeline(l_VkPipeline);

          return true;
        }

        bool register_graphics_pipeline(
            Context &p_Context, Pipeline p_Pipeline,
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

          compile_graphics_pipeline(p_Context, p_Pipeline);

          return true;
        }

        bool do_tick(Context &p_Context, float p_Delta)
        {
          for (auto it = g_SourceTimes.begin();
               it != g_SourceTimes.end(); ++it) {
            Util::String i_SourcePath = it->first;

            uint64_t i_Modified =
                Util::FileIO::modified_sync(i_SourcePath.c_str());

            if (i_Modified == it->second) {
              continue;
            }

            vkDeviceWaitIdle(p_Context.device);

            g_SourceTimes[i_SourcePath] = i_Modified;

            auto i_GraphicsSourcesEntry =
                g_GraphicsSources.find(i_SourcePath);
            if (i_GraphicsSourcesEntry != g_GraphicsSources.end()) {
              Pipeline i_Pipeline = i_GraphicsSourcesEntry->second;

              compile_graphics_pipeline(p_Context, i_Pipeline);
            }
          }

          return true;
        }

        bool tick(Context &p_Context, float p_Delta)
        {
          // LOW_PROFILE_CPU("Renderer", "Pipeline manager");
          // #if LOW_RENDERER_COMPILE_SHADERS
          static float l_Count = 100.0f;
          if (l_Count > 1.0f) {
            do_tick(p_Context, p_Delta);
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
