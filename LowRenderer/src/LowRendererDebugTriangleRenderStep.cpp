#include "LowRendererCustomRenderSteps.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "LowMathVectorUtil.h"

#include "LowRendererGraphicsStepConfig.h"
#include "LowRendererInterface.h"
#include "LowRendererBackend.h"
#include "LowRendererGraphicsStep.h"
#include "LowRenderer.h"
#include "LowRendererDynamicBuffer.h"

namespace Low {
  namespace Renderer {
    namespace DebugTriangleStep {
      void execute(GraphicsStep p_Step, RenderFlow p_RenderFlow,
                   Math::Matrix4x4 &p_ProjectionMatrix,
                   Math::Matrix4x4 &p_ViewMatrix)
      {
        p_Step.get_renderpasses()[p_RenderFlow].begin();

        get_debug_geometry_triangle_vertex_buffer().bind();

        uint32_t l_Triangles =
            get_debug_geometry_triangle_vertex_buffer().get_used_elements() / 3;

        for (auto pit = p_Step.get_pipelines()[p_RenderFlow].begin();
             pit != p_Step.get_pipelines()[p_RenderFlow].end(); ++pit) {
          Interface::GraphicsPipeline i_Pipeline = *pit;
          i_Pipeline.bind();

          for (uint32_t i = 0; i < l_Triangles; ++i) {
            Backend::DrawParams i_Params;
            i_Params.context = &p_Step.get_context().get_context();
            i_Params.vertexCount = 3;
            i_Params.firstVertex = i * 3;

            Backend::callbacks().draw(i_Params);
          }
        }

        p_Step.get_renderpasses()[p_RenderFlow].end();
      }

      void setup_config()
      {
        GraphicsStepConfig l_Config =
            GraphicsStepConfig::make(N(DebugTrianglePass));
        l_Config.get_dimensions_config().type =
            ImageResourceDimensionType::RELATIVE;
        l_Config.get_dimensions_config().relative.multiplier = 1.0f;
        l_Config.get_dimensions_config().relative.target =
            ImageResourceDimensionRelativeOptions::RENDERFLOW;

        {
          l_Config.set_depth_clear(false);
          l_Config.set_depth_compare_operation(Backend::CompareOperation::LESS);
          l_Config.set_depth_test(true);
          l_Config.set_depth_write(true);
          l_Config.set_use_depth(true);

          PipelineResourceBindingConfig l_ResourceBinding;
          l_ResourceBinding.resourceName = N(GBufferDepth);
          l_ResourceBinding.resourceScope = ResourceBindScope::RENDERFLOW;
          l_ResourceBinding.bindType = ResourceBindType::IMAGE;
          l_Config.set_depth_rendertarget(l_ResourceBinding);
        }

        {
          PipelineResourceBindingConfig l_ResourceBinding;
          l_ResourceBinding.resourceName = N(HoverHighlightImage);
          l_ResourceBinding.resourceScope = ResourceBindScope::RENDERFLOW;
          l_ResourceBinding.bindType = ResourceBindType::IMAGE;
          l_Config.get_rendertargets().push_back(l_ResourceBinding);
        }

        {
          GraphicsPipelineConfig l_PipelineConfig =
              get_graphics_pipeline_config(N(debuggeometry_triangles));

          if (l_PipelineConfig.cullMode ==
              Backend::PipelineRasterizerCullMode::BACK) {
            l_PipelineConfig.cullMode =
                Backend::PipelineRasterizerCullMode::FRONT;
          } else {
            l_PipelineConfig.cullMode =
                Backend::PipelineRasterizerCullMode::BACK;
          }

          l_Config.get_pipelines().push_back(l_PipelineConfig);
        }

        l_Config.get_callbacks().setup_signature =
            &GraphicsStep::create_signature;
        l_Config.get_callbacks().setup_pipelines =
            &GraphicsStep::create_pipelines;
        l_Config.get_callbacks().setup_renderpass =
            &GraphicsStep::create_renderpass;
        l_Config.get_callbacks().execute = &execute;

        {
          ResourceConfig l_ResourceConfig;
          l_ResourceConfig.arraySize = 1;
          l_ResourceConfig.name = N(_renderobject_buffer);
          l_ResourceConfig.type = ResourceType::BUFFER;
          l_ResourceConfig.buffer.size =
              sizeof(RenderObjectShaderInfo) * LOW_RENDERER_RENDEROBJECT_COUNT;
          l_Config.get_resources().push_back(l_ResourceConfig);
        }
        {
          ResourceConfig l_ResourceConfig;
          l_ResourceConfig.arraySize = 1;
          l_ResourceConfig.name = N(_color_buffer);
          l_ResourceConfig.type = ResourceType::BUFFER;
          l_ResourceConfig.buffer.size = sizeof(Math::Vector4) * 32u;
          l_Config.get_resources().push_back(l_ResourceConfig);
        }
      }
    } // namespace DebugTriangleStep
  }   // namespace Renderer
} // namespace Low
