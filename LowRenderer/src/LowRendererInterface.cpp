#include "LowRendererInterface.h"

#include "LowUtilFileIO.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      namespace PipelineManager {
        struct GraphicsPipelineOutputPaths
        {
          Util::String vertex;
          Util::String fragment;
        };

        static void recreate_compute_pipeline(ComputePipeline p_Pipeline,
                                              bool p_CleanOld = true);
        static void recreate_graphics_pipeline(GraphicsPipeline p_Pipeline,
                                               bool p_CleanOld = true);

        Util::Map<GraphicsPipeline, Backend::PipelineGraphicsCreateParams>
            g_GraphicsPipelineParams;
        Util::Map<GraphicsPipeline, GraphicsPipelineOutputPaths>
            g_GraphicsPipelineOutPaths;

        Util::Map<ComputePipeline, Backend::PipelineComputeCreateParams>
            g_ComputePipelineParams;
        Util::Map<ComputePipeline, Util::String> g_ComputePipelineOutPaths;

        Util::Map<Util::String, Util::List<GraphicsPipeline>>
            g_GraphicsPipelines;
        Util::Map<Util::String, Util::List<ComputePipeline>> g_ComputePipelines;

        Util::Map<Util::String, uint64_t> g_SourceTimes;

        Util::String compile(Util::String p_Path)
        {
          Util::String l_SourcePath =
              Backend::callbacks().get_shader_source_path(p_Path);
          uint64_t l_ModifiedTime =
              Util::FileIO::modified_sync(l_SourcePath.c_str());

          g_SourceTimes[p_Path] = l_ModifiedTime;

          return Backend::callbacks().compile(p_Path);
        }

        void register_compute_pipeline(
            ComputePipeline p_Pipeline,
            Backend::PipelineComputeCreateParams &p_Params)
        {
          g_ComputePipelineParams[p_Pipeline] = p_Params;

          Util::String l_OutputPath = compile(p_Params.shaderPath);

          if (g_ComputePipelines.find(l_OutputPath) ==
              g_ComputePipelines.end()) {
            g_ComputePipelines[l_OutputPath] = Util::List<ComputePipeline>();
          }

          g_ComputePipelines[l_OutputPath].push_back(p_Pipeline);

          g_ComputePipelineOutPaths[p_Pipeline] = l_OutputPath;

          recreate_compute_pipeline(p_Pipeline, false);
        }

        static void recreate_compute_pipeline(ComputePipeline p_Pipeline,
                                              bool p_CleanOld)
        {
          if (p_CleanOld) {
            Backend::callbacks().pipeline_cleanup(p_Pipeline.get_pipeline());
          }

          Backend::PipelineComputeCreateParams l_Params =
              g_ComputePipelineParams[p_Pipeline];

          Util::String l_OutputPath = g_ComputePipelineOutPaths[p_Pipeline];

          l_Params.shaderPath = l_OutputPath.c_str();

          Backend::callbacks().pipeline_compute_create(
              p_Pipeline.get_pipeline(), l_Params);
        }

        static void recreate_graphics_pipeline(GraphicsPipeline p_Pipeline,
                                               bool p_CleanOld)
        {
          if (p_CleanOld) {
            Backend::callbacks().pipeline_cleanup(p_Pipeline.get_pipeline());
          }

          Backend::PipelineGraphicsCreateParams l_Params =
              g_GraphicsPipelineParams[p_Pipeline];

          GraphicsPipelineOutputPaths &l_OutputPaths =
              g_GraphicsPipelineOutPaths[p_Pipeline];

          // TODO!!

          l_Params.vertexShaderPath = l_OutputPaths.vertex.c_str();
          l_Params.fragmentShaderPath = l_OutputPaths.fragment.c_str();

          Backend::callbacks().pipeline_graphics_create(
              p_Pipeline.get_pipeline(), l_Params);
        }

        static void recreate_pipeline(Util::String p_OutPath)
        {

          bool l_Waited = false;

          for (uint32_t i = 0; i < g_GraphicsPipelines[p_OutPath].size(); ++i) {
            GraphicsPipeline i_Pipeline = g_GraphicsPipelines[p_OutPath][i];

            if (!l_Waited) {
              Backend::callbacks().context_wait_idle(
                  *i_Pipeline.get_pipeline().context);
              l_Waited = true;
            }

            recreate_graphics_pipeline(i_Pipeline);
          }

          for (uint32_t i = 0; i < g_ComputePipelines[p_OutPath].size(); ++i) {
            ComputePipeline i_Pipeline = g_ComputePipelines[p_OutPath][i];

            if (!l_Waited) {
              Backend::callbacks().context_wait_idle(
                  *i_Pipeline.get_pipeline().context);
              l_Waited = true;
            }

            recreate_compute_pipeline(i_Pipeline);
          }
        }

        static void do_tick(float p_Delta)
        {
          for (auto it = g_SourceTimes.begin(); it != g_SourceTimes.end();
               ++it) {
            Util::String i_SourcePath =
                Backend::callbacks().get_shader_source_path(it->first);

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

        void delist_compute_pipeline(ComputePipeline p_Pipeline)
        {
          g_ComputePipelineParams.erase(p_Pipeline);

          Util::String l_OutputPath = g_ComputePipelineOutPaths[p_Pipeline];

          g_ComputePipelines[l_OutputPath].erase_first(p_Pipeline);

          g_ComputePipelineOutPaths.erase(p_Pipeline);
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
      } // namespace PipelineManager
    }   // namespace Interface
  }     // namespace Renderer
} // namespace Low
