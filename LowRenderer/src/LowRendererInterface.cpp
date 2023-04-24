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

        Util::Map<GraphicsPipeline, PipelineGraphicsCreateParams>
            g_GraphicsPipelineParams;
        Util::Map<GraphicsPipeline, GraphicsPipelineOutputPaths>
            g_GraphicsPipelineOutPaths;

        Util::Map<ComputePipeline, PipelineComputeCreateParams>
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

        void register_compute_pipeline(ComputePipeline p_Pipeline,
                                       PipelineComputeCreateParams &p_Params)
        {
          bool l_AlreadyExisting = false;
          for (auto it = g_ComputePipelineParams.begin();
               it != g_ComputePipelineParams.end(); ++it) {
            if (it->first == p_Pipeline) {
              l_AlreadyExisting = true;
              break;
            }
          }

          g_ComputePipelineParams[p_Pipeline] = p_Params;

          if (!l_AlreadyExisting) {
            Util::String l_OutputPath = compile(p_Params.shaderPath);

            if (g_ComputePipelines.find(l_OutputPath) ==
                g_ComputePipelines.end()) {
              g_ComputePipelines[l_OutputPath] = Util::List<ComputePipeline>();
            }

            g_ComputePipelines[l_OutputPath].push_back(p_Pipeline);

            g_ComputePipelineOutPaths[p_Pipeline] = l_OutputPath;
          }

          recreate_compute_pipeline(p_Pipeline, l_AlreadyExisting);
        }

        static void recreate_compute_pipeline(ComputePipeline p_Pipeline,
                                              bool p_CleanOld)
        {
          if (p_CleanOld) {
            Backend::callbacks().pipeline_cleanup(p_Pipeline.get_pipeline());
          }

          PipelineComputeCreateParams &l_Params =
              g_ComputePipelineParams[p_Pipeline];

          Backend::PipelineComputeCreateParams l_BeParams;
          l_BeParams.context = &l_Params.context.get_context();
          Util::String l_OutputPath = g_ComputePipelineOutPaths[p_Pipeline];
          l_BeParams.shaderPath = l_OutputPath.c_str();
          l_BeParams.signatureCount =
              static_cast<uint8_t>(l_Params.signatures.size());
          Util::List<Backend::PipelineResourceSignature> l_Signatures;
          for (uint8_t i = 0; i < l_BeParams.signatureCount; ++i) {
            l_Signatures.push_back(l_Params.signatures[i].get_signature());
          }
          l_BeParams.signatures = l_Signatures.data();

          Backend::callbacks().pipeline_compute_create(
              p_Pipeline.get_pipeline(), l_BeParams);
        }

        void register_graphics_pipeline(GraphicsPipeline p_Pipeline,
                                        PipelineGraphicsCreateParams &p_Params)
        {
          bool l_AlreadyExisting = false;
          for (auto it = g_GraphicsPipelineParams.begin();
               it != g_GraphicsPipelineParams.end(); ++it) {
            if (it->first == p_Pipeline) {
              l_AlreadyExisting = true;
              break;
            }
          }

          g_GraphicsPipelineParams[p_Pipeline] = p_Params;

          if (!l_AlreadyExisting) {
            GraphicsPipelineOutputPaths l_OutputPaths;
            l_OutputPaths.vertex = compile(p_Params.vertexShaderPath);
            l_OutputPaths.fragment = compile(p_Params.fragmentShaderPath);

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
          }

          recreate_graphics_pipeline(p_Pipeline, l_AlreadyExisting);
        }

        static void recreate_graphics_pipeline(GraphicsPipeline p_Pipeline,
                                               bool p_CleanOld)
        {
          if (p_CleanOld) {
            Backend::callbacks().pipeline_cleanup(p_Pipeline.get_pipeline());
          }

          PipelineGraphicsCreateParams l_Params =
              g_GraphicsPipelineParams[p_Pipeline];

          GraphicsPipelineOutputPaths &l_OutputPaths =
              g_GraphicsPipelineOutPaths[p_Pipeline];

          Backend::PipelineGraphicsCreateParams l_BeParams;
          l_BeParams.context = &l_Params.context.get_context();
          l_BeParams.vertexShaderPath = l_OutputPaths.vertex.c_str();
          l_BeParams.fragmentShaderPath = l_OutputPaths.fragment.c_str();
          l_BeParams.dimensions = l_Params.dimensions;
          l_BeParams.signatureCount =
              static_cast<uint8_t>(l_Params.signatures.size());
          Util::List<Backend::PipelineResourceSignature> l_Signatures;
          for (uint8_t i = 0; i < l_BeParams.signatureCount; ++i) {
            l_Signatures.push_back(l_Params.signatures[i].get_signature());
          }
          l_BeParams.signatures = l_Signatures.data();
          l_BeParams.cullMode = l_Params.cullMode;
          l_BeParams.frontFace = l_Params.frontFace;
          l_BeParams.polygonMode = l_Params.polygonMode;
          l_BeParams.renderpass = &l_Params.renderpass.get_renderpass();
          l_BeParams.colorTargetCount =
              static_cast<uint8_t>(l_Params.colorTargets.size());
          l_BeParams.colorTargets = l_Params.colorTargets.data();
          l_BeParams.vertexDataAttributeCount =
              static_cast<uint8_t>(l_Params.vertexDataAttributeTypes.size());
          l_BeParams.vertexDataAttributesType =
              l_Params.vertexDataAttributeTypes.data();
          l_BeParams.depthTest = l_Params.depthTest;
          l_BeParams.depthWrite = l_Params.depthWrite;
          l_BeParams.depthCompareOperation = l_Params.depthCompareOperation;

          Backend::callbacks().pipeline_graphics_create(
              p_Pipeline.get_pipeline(), l_BeParams);
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
#if LOW_RENDERER_COMPILE_SHADERS
          do_tick(p_Delta);
#endif
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
