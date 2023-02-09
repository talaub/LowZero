#include "LowRendererInterface.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilFileIO.h"

#include "LowRendererBackend.h"

namespace Low {
  namespace Renderer {
    namespace Interface {

      void draw(DrawParams &p_Params)
      {
        Backend::DrawParams l_Params;
        l_Params.commandBuffer = &(p_Params.commandBuffer.get_commandbuffer());
        l_Params.firstInstance = p_Params.firstInstance;
        l_Params.firstVertex = p_Params.firstVertex;
        l_Params.vertexCount = p_Params.vertexCount;
        l_Params.instanceCount = p_Params.instanceCount;

        Backend::draw(l_Params);
      }

      namespace ShaderProgramUtils {
        struct GraphicsPipelineOutputPaths
        {
          Util::String vertex;
          Util::String fragment;
        };

        Util::Map<GraphicsPipeline, GraphicsPipelineCreateParams>
            g_GraphicsPipelineParams;
        Util::Map<GraphicsPipeline, GraphicsPipelineOutputPaths>
            g_GraphicsPipelineOutPaths;

        Util::Map<Util::String, Util::List<GraphicsPipeline>>
            g_GraphicsPipelines;

        Util::Map<Util::String, uint64_t> g_SourceTimes;

        static void recreate_graphics_pipeline(GraphicsPipeline p_Pipeline,
                                               bool p_CleanOld = true);

        static Util::String get_source_path_vk_glsl(Util::String p_Path)
        {
          return Util::String(LOW_DATA_PATH) + "/shader/src/vk_glsl/" + p_Path;
        }

        static Util::String get_source_path(Util::String p_Path)
        {
#ifdef LOW_RENDERER_API_VULKAN
          return get_source_path_vk_glsl(p_Path);
#else
          LOW_ASSERT(false, "No graphic api defined");
          return "";
#endif
        }

        static Util::String compile_vk_glsl_to_spv(Util::String p_Path)
        {
          Util::String l_SourcePath = get_source_path(p_Path);
          Util::String l_OutPath = Util::String(LOW_DATA_PATH) +
                                   "/shader/dst/spv/" + p_Path + ".spv";

          Util::String l_Command = "glslc " + l_SourcePath + " -o " + l_OutPath;

          Util::String l_Notice = "Compiling shader " + l_SourcePath;

          LOW_LOG_DEBUG(l_Notice.c_str());
          system(l_Command.c_str());

          return l_OutPath;
        }

        Util::String compile(Util::String p_Path)
        {
          Util::String l_SourcePath = get_source_path(p_Path);
          uint64_t l_ModifiedTime =
              Util::FileIO::modified_sync(l_SourcePath.c_str());

          g_SourceTimes[p_Path] = l_ModifiedTime;

#ifdef LOW_RENDERER_API_VULKAN
          return compile_vk_glsl_to_spv(p_Path);
#else
          LOW_ASSERT(false, "No graphic api defined");
          return "";
#endif
        }

        void register_graphics_pipeline(GraphicsPipeline p_Pipeline,
                                        GraphicsPipelineCreateParams &p_Params)
        {
          g_GraphicsPipelineParams[p_Pipeline] = p_Params;

          GraphicsPipelineOutputPaths l_OutputPaths;
          l_OutputPaths.vertex = compile(p_Params.vertexPath);
          l_OutputPaths.fragment = compile(p_Params.fragmentPath);

          if (g_GraphicsPipelines.find(l_OutputPaths.vertex) ==
              g_GraphicsPipelines.end()) {
            g_GraphicsPipelines[l_OutputPaths.vertex] =
                Util::List<GraphicsPipeline>();
          }
          if (g_GraphicsPipelines.find(l_OutputPaths.fragment) ==
              g_GraphicsPipelines.end()) {
            g_GraphicsPipelines[l_OutputPaths.fragment] =
                Util::List<GraphicsPipeline>();
          }
          g_GraphicsPipelines[l_OutputPaths.vertex].push_back(p_Pipeline);
          g_GraphicsPipelines[l_OutputPaths.fragment].push_back(p_Pipeline);

          g_GraphicsPipelineOutPaths[p_Pipeline] = l_OutputPaths;

          recreate_graphics_pipeline(p_Pipeline, false);
        }

        void delist_graphics_pipeline(GraphicsPipeline p_Pipeline)
        {
          g_GraphicsPipelineParams.erase(p_Pipeline);

          GraphicsPipelineOutputPaths &l_OutputPaths =
              g_GraphicsPipelineOutPaths[p_Pipeline];

          g_GraphicsPipelines[l_OutputPaths.vertex].erase_first(p_Pipeline);
          g_GraphicsPipelines[l_OutputPaths.fragment].erase_first(p_Pipeline);

          g_GraphicsPipelineOutPaths.erase(p_Pipeline);
        }

        static void recreate_graphics_pipeline(GraphicsPipeline p_Pipeline,
                                               bool p_CleanOld)
        {
          if (p_CleanOld) {
            Backend::pipeline_cleanup(p_Pipeline.get_pipeline());
          }

          GraphicsPipelineCreateParams &l_Params =
              g_GraphicsPipelineParams[p_Pipeline];

          GraphicsPipelineOutputPaths &l_OutputPaths =
              g_GraphicsPipelineOutPaths[p_Pipeline];

          Backend::GraphicsPipelineCreateParams l_BeParams;
          l_BeParams.context = &(l_Params.context.get_context());
          l_BeParams.vertexShaderPath = l_OutputPaths.vertex.c_str();
          l_BeParams.fragmentShaderPath = l_OutputPaths.fragment.c_str();
          l_BeParams.interface = &(l_Params.interface.get_interface());
          l_BeParams.dimensions = l_Params.dimensions;
          l_BeParams.colorTargetCount = l_Params.colorTargets.size();
          l_BeParams.colorTargets = l_Params.colorTargets.data();
          l_BeParams.cullMode = l_Params.cullMode;
          l_BeParams.frontFace = l_Params.frontFace;
          l_BeParams.polygonMode = l_Params.polygonMode;
          l_BeParams.renderpass = &(l_Params.renderpass.get_renderpass());
          l_BeParams.vertexInput = l_Params.vertexInput;

          Backend::pipeline_graphics_create(p_Pipeline.get_pipeline(),
                                            l_BeParams);

          // TL TODO: Create pipeline
        }

        static void recreate_pipeline(Util::String p_OutPath)
        {
          if (g_GraphicsPipelines.find(p_OutPath) ==
              g_GraphicsPipelines.end()) {
            return;
          }

          for (uint32_t i = 0; i < g_GraphicsPipelines[p_OutPath].size(); ++i) {
            GraphicsPipeline i_Pipeline = g_GraphicsPipelines[p_OutPath][i];

            recreate_graphics_pipeline(i_Pipeline);
          }
        }

        static void do_tick(float p_Delta)
        {
          for (auto it = g_SourceTimes.begin(); it != g_SourceTimes.end();
               ++it) {
            Util::String i_SourcePath = get_source_path(it->first);

            uint64_t i_Modified =
                Util::FileIO::modified_sync(i_SourcePath.c_str());

            if (i_Modified == it->second) {
              continue;
            }

            Util::String i_OutPath = compile(it->first);
            recreate_pipeline(i_OutPath);
          }
        }

        void tick(float p_Delta)
        {
          do_tick(p_Delta);
        }
      } // namespace ShaderProgramUtils

      namespace UniformPoolUtils {
#define POOL_MINIMUM 32u
        UniformPool g_UniformPool;
        UniformPoolCreateParams g_Params;

        static void create_uniform_pool(UniformPoolCreateParams p_Params)
        {
          if (p_Params.scopeCount < 128u) {
            p_Params.scopeCount = 128u;
          }
          p_Params.rendertargetCount *= 2;
          p_Params.samplerCount *= 2;
          p_Params.uniformBufferCount *= 2;
          p_Params.storageBufferCount *= 2;

          p_Params.rendertargetCount =
              LOW_MATH_MAX(p_Params.rendertargetCount, POOL_MINIMUM);
          p_Params.samplerCount =
              LOW_MATH_MAX(p_Params.samplerCount, POOL_MINIMUM);
          p_Params.uniformBufferCount =
              LOW_MATH_MAX(p_Params.uniformBufferCount, POOL_MINIMUM);
          p_Params.storageBufferCount =
              LOW_MATH_MAX(p_Params.storageBufferCount, POOL_MINIMUM);

          g_UniformPool = UniformPool::make(N(InternalUniformPool), p_Params);
          g_Params = p_Params;
        }

        static bool check_uniform_pool(UniformPoolCreateParams &p_Params)
        {
          return g_Params.rendertargetCount >= p_Params.rendertargetCount &&
                 g_Params.samplerCount >= p_Params.samplerCount &&
                 g_Params.uniformBufferCount >= p_Params.uniformBufferCount &&
                 g_Params.storageBufferCount >= p_Params.storageBufferCount;
        }

        UniformPool get_uniform_pool(UniformPoolCreateParams &p_Params)
        {
          if (!g_UniformPool.is_alive()) {
            create_uniform_pool(p_Params);
          }

          if (!check_uniform_pool(p_Params)) {
            create_uniform_pool(p_Params);
          }

          g_Params.rendertargetCount -= p_Params.rendertargetCount;
          g_Params.samplerCount -= p_Params.samplerCount;
          g_Params.uniformBufferCount -= p_Params.uniformBufferCount;
          g_Params.storageBufferCount -= p_Params.storageBufferCount;

          return g_UniformPool;
        }
#undef POOL_MINIMUM
      } // namespace UniformPoolUtils
    }   // namespace Interface
  }     // namespace Renderer
} // namespace Low
