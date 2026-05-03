#include "LowRendererVulkanPipelineManager.h"

#include "LowUtilFileIO.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"

#include "LowUtil.h"

#include "LowRendererShaderSource.h"

#ifndef LOW_RENDERER_COMPILE_SHADERS
#define LOW_RENDERER_COMPILE_SHADERS 0
#endif

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace PipelineManager {
        Util::Map<Pipeline, PipelineUtil::GraphicsPipelineBuilder>
            g_GraphicsPipelines;
        Util::Map<Pipeline, PipelineUtil::ComputePipelineBuilder>
            g_ComputePipelines;

        Util::Map<Util::String, Util::List<Pipeline>> g_Sources;

        Util::Map<Util::String, u64> g_SourceTimes;

        struct VariantCompileState
        {
          u64 sourceModified = 0;
          u64 compilePass = 0;
        };

        Util::Map<u64, VariantCompileState> g_VariantCompileStates;
        u64 g_CompilePass = 1;

        static void begin_compile_pass()
        {
          ++g_CompilePass;
        }

        static bool has_pipeline(Util::List<Pipeline> &p_Pipelines,
                                 Pipeline p_Pipeline)
        {
          for (auto it = p_Pipelines.begin(); it != p_Pipelines.end();
               ++it) {
            if (it->get_id() == p_Pipeline.get_id()) {
              return true;
            }
          }

          return false;
        }

        static void
        register_pipeline_source(Util::String p_SourcePath,
                                 Pipeline p_Pipeline)
        {
          if (!has_pipeline(g_Sources[p_SourcePath], p_Pipeline)) {
            g_Sources[p_SourcePath].push_back(p_Pipeline);
          }

          if (g_SourceTimes.find(p_SourcePath) ==
              g_SourceTimes.end()) {
            g_SourceTimes[p_SourcePath] =
                Util::FileIO::modified_sync(p_SourcePath.c_str());
          }
        }

        static void
        register_dependent_pipeline(ShaderVariant p_Variant,
                                    Pipeline p_Pipeline)
        {
          if (!p_Variant.is_alive()) {
            return;
          }

          Util::List<uint64_t> &l_DependentPipelines =
              p_Variant.get_dependent_pipelines();

          for (auto it = l_DependentPipelines.begin();
               it != l_DependentPipelines.end(); ++it) {
            if (*it == p_Pipeline.get_id()) {
              return;
            }
          }

          l_DependentPipelines.push_back(p_Pipeline.get_id());
        }

        // Temporary copy of the compileshaders function to support
        // legacy pipelines
        // TODO: Merge both implementations of compile_shader
        static bool compile_shader(const Util::String p_SourcePath,
                                   const Util::String p_OutPath)
        {
#if LOW_RENDERER_COMPILE_SHADERS
          Util::String l_IncludeCommand =
              "-I " + Util::get_project().engineDataPath +
              "\\lowr_shaders\\lib";
          Util::String l_Command =
              "glslc " + l_IncludeCommand + " " + p_SourcePath;

          l_Command += " -o" + p_OutPath;

          Util::String l_Notice = "Compiling shader " + p_SourcePath;

          LOW_LOG_DEBUG << l_Notice << LOW_LOG_END;
          Util::execute_command(l_Command);
#endif

          return true;
        }

        static bool compile_shader(ShaderVariant p_Variant,
                                   const Util::String p_SourcePath,
                                   const Util::String p_OutPath)
        {
          const u64 l_SourceModified =
              Util::FileIO::modified_sync(p_SourcePath.c_str());

          VariantCompileState &l_State =
              g_VariantCompileStates[p_Variant.get_id()];

          if (l_State.compilePass == g_CompilePass) {
            return true;
          }

          l_State.compilePass = g_CompilePass;

          if (l_State.sourceModified == l_SourceModified &&
              Util::FileIO::file_exists_sync(p_OutPath.c_str())) {
            return true;
          }

#if LOW_RENDERER_COMPILE_SHADERS
          Util::String l_IncludeCommand =
              "-I " + Util::get_project().engineDataPath +
              "\\lowr_shaders\\lib";
          Util::String l_Command =
              "glslc " + l_IncludeCommand + " " + p_SourcePath;

          for (ShaderDefine &i_Define : p_Variant.get_defines()) {
            l_Command += " -D ";
            l_Command += i_Define.name.c_str();

            if (!i_Define.value.empty()) {
              l_Command += "=";
              l_Command += i_Define.value;
            }
          }
          l_Command += " -o" + p_OutPath;

          Util::String l_Notice = "Compiling shader " + p_SourcePath;

          LOW_LOG_DEBUG << l_Notice << LOW_LOG_END;
          Util::execute_command(l_Command);
#endif

          l_State.sourceModified = l_SourceModified;

          return true;
        }

        bool compile_graphics_pipeline(Pipeline p_Pipeline,
                                       bool p_CompileShaders)
        {
          PipelineUtil::GraphicsPipelineBuilder &l_Builder =
              g_GraphicsPipelines[p_Pipeline];

          // Destroy old pipeline
          vkDestroyPipeline(Global::get_device(), p_Pipeline.get(),
                            nullptr);

          if (p_CompileShaders) {
            if (l_Builder.vertexShader.is_alive() &&
                l_Builder.fragmentShader.is_alive()) {
              compile_shader(l_Builder.vertexShader,
                             l_Builder.vertexShaderPath,
                             l_Builder.vertexSpirvPath);
              compile_shader(l_Builder.fragmentShader,
                             l_Builder.fragmentShaderPath,
                             l_Builder.fragmentSpirvPath);
            } else {
              compile_shader(l_Builder.vertexShaderPath,
                             l_Builder.vertexSpirvPath);
              compile_shader(l_Builder.fragmentShaderPath,
                             l_Builder.fragmentSpirvPath);
            }
          }

          // Update shaders in builder
          l_Builder.update_shaders();

          VkPipeline l_VkPipeline =
              l_Builder.build_pipeline(Global::get_device());

          // Create new pipeline and assign
          p_Pipeline.set(l_VkPipeline);

          return true;
        }

        bool compile_compute_pipeline(Pipeline p_Pipeline,
                                      bool p_CompileShaders)
        {
          PipelineUtil::ComputePipelineBuilder &l_Builder =
              g_ComputePipelines[p_Pipeline];

          // Destroy old pipeline
          vkDestroyPipeline(Global::get_device(), p_Pipeline.get(),
                            nullptr);

          if (p_CompileShaders) {
            compile_shader(l_Builder.computeShaderPath,
                           l_Builder.computeSpirvPath);
          }

          // Update shaders in builder
          l_Builder.update_shader();

          VkPipeline l_VkPipeline =
              l_Builder.build_pipeline(Global::get_device());

          // Create new pipeline and assign
          p_Pipeline.set(l_VkPipeline);

          return true;
        }

        bool register_graphics_pipeline(
            Pipeline p_Pipeline,
            PipelineUtil::GraphicsPipelineBuilder p_Builder)
        {
          g_GraphicsPipelines[p_Pipeline] = p_Builder;

          register_pipeline_source(p_Builder.vertexShaderPath,
                                   p_Pipeline);
          register_pipeline_source(p_Builder.fragmentShaderPath,
                                   p_Pipeline);

          register_dependent_pipeline(p_Builder.vertexShader,
                                      p_Pipeline);
          register_dependent_pipeline(p_Builder.fragmentShader,
                                      p_Pipeline);

          begin_compile_pass();
          compile_graphics_pipeline(p_Pipeline);

          return true;
        }

        bool register_compute_pipeline(
            Pipeline p_Pipeline,
            PipelineUtil::ComputePipelineBuilder p_Builder)
        {
          g_ComputePipelines[p_Pipeline] = p_Builder;

          register_pipeline_source(p_Builder.computeShaderPath,
                                   p_Pipeline);

          begin_compile_pass();
          compile_compute_pipeline(p_Pipeline);
          return true;
        }

        bool do_tick(float p_Delta)
        {
          begin_compile_pass();

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

            auto i_SourcesEntry = g_Sources.find(i_SourcePath);
            if (i_SourcesEntry != g_Sources.end()) {
              for (auto i_PipelineIt = i_SourcesEntry->second.begin();
                   i_PipelineIt != i_SourcesEntry->second.end();
                   ++i_PipelineIt) {
                Pipeline i_Pipeline = *i_PipelineIt;

                auto i_GraphicsEntry =
                    g_GraphicsPipelines.find(i_Pipeline);
                auto i_ComputeEntry =
                    g_ComputePipelines.find(i_Pipeline);
                if (i_GraphicsEntry != g_GraphicsPipelines.end()) {
                  compile_graphics_pipeline(i_Pipeline, true);
                } else if (i_ComputeEntry !=
                           g_ComputePipelines.end()) {
                  compile_compute_pipeline(i_Pipeline, true);
                }
              }
            }
          }

          return true;
        }

        bool tick(float p_Delta)
        {
          // LOW_PROFILE_CPU("Renderer", "Pipeline manager");
#if LOW_RENDERER_COMPILE_SHADERS
          static float l_Count = 100.0f;
          if (l_Count > 1.0f) {
            do_tick(p_Delta);
            l_Count = 0.0f;
          }
          l_Count += p_Delta;
#endif

          return true;
        }
      } // namespace PipelineManager
    } // namespace Vulkan
  } // namespace Renderer
} // namespace Low
