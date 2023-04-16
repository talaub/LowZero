#include "LowRendererCustomRenderSteps.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include "LowMathVectorUtil.h"

#include "LowRendererGraphicsStepConfig.h"
#include "LowRendererInterface.h"
#include "LowRendererBackend.h"
#include "LowRendererGraphicsStep.h"
#include "LowRenderer.h"

namespace Low {
  namespace Renderer {
    namespace DebugGeometryStep {
      void setup_signature(GraphicsStep p_Step, RenderFlow p_RenderFlow)
      {
        Util::List<Backend::PipelineResourceDescription> l_ResourceDescriptions;

        {
          Backend::PipelineResourceDescription l_ResourceDescription;
          l_ResourceDescription.arraySize = 1;
          l_ResourceDescription.name = N(u_RenderObjects);
          l_ResourceDescription.step = Backend::ResourcePipelineStep::GRAPHICS;
          l_ResourceDescription.type = Backend::ResourceType::BUFFER;

          l_ResourceDescriptions.push_back(l_ResourceDescription);
        }
        {
          Backend::PipelineResourceDescription l_ResourceDescription;
          l_ResourceDescription.arraySize = 1;
          l_ResourceDescription.name = N(u_Colors);
          l_ResourceDescription.step = Backend::ResourcePipelineStep::GRAPHICS;
          l_ResourceDescription.type = Backend::ResourceType::BUFFER;

          l_ResourceDescriptions.push_back(l_ResourceDescription);
        }

        p_Step.get_signatures()[p_RenderFlow] =
            Interface::PipelineResourceSignature::make(N(StepResourceSignature),
                                                       p_Step.get_context(), 2,
                                                       l_ResourceDescriptions);

        p_Step.get_signatures()[p_RenderFlow].set_buffer_resource(
            N(u_RenderObjects), 0,
            p_Step.get_resources()[p_RenderFlow].get_buffer_resource(
                N(_renderobject_buffer)));

        p_Step.get_signatures()[p_RenderFlow].set_buffer_resource(
            N(u_Colors), 0,
            p_Step.get_resources()[p_RenderFlow].get_buffer_resource(
                N(color_buffer)));
      }

      void execute(GraphicsStep p_Step, RenderFlow p_RenderFlow,
                   Math::Matrix4x4 &p_ProjectionMatrix,
                   Math::Matrix4x4 &p_ViewMatrix)
      {
        p_Step.get_renderpasses()[p_RenderFlow].begin();

        RenderObjectShaderInfo l_ObjectShaderInfos[32];
        Math::Vector4 l_Colors[32];
        uint32_t l_ObjectIndex = 0;

        for (auto pit = p_Step.get_pipelines()[p_RenderFlow].begin();
             pit != p_Step.get_pipelines()[p_RenderFlow].end(); ++pit) {
          Interface::GraphicsPipeline i_Pipeline = *pit;
          i_Pipeline.bind();

          for (auto mit = p_Step.get_renderobjects()[pit->get_name()].begin();
               mit != p_Step.get_renderobjects()[pit->get_name()].end();
               ++mit) {
            for (auto it = mit->second.begin(); it != mit->second.end();) {
              Math::Matrix4x4 l_ModelMatrix =
                  glm::translate(glm::mat4(1.0f), it->world_position) *
                  glm::toMat4(it->world_rotation) *
                  glm::scale(glm::mat4(1.0f), it->world_scale);

              Math::Matrix4x4 l_MVPMatrix =
                  p_ProjectionMatrix * p_ViewMatrix * l_ModelMatrix;

              l_ObjectShaderInfos[l_ObjectIndex].mvp = l_MVPMatrix;
              l_Colors[l_ObjectIndex] = it->color;

              l_ObjectIndex++;

              ++it;
            }
          }
        }

        p_Step.get_resources()[p_RenderFlow]
            .get_buffer_resource(N(_renderobject_buffer))
            .set((void *)l_ObjectShaderInfos);
        p_Step.get_resources()[p_RenderFlow]
            .get_buffer_resource(N(color_buffer))
            .set((void *)l_Colors);

        GraphicsStep::draw_renderobjects(p_Step, p_RenderFlow);

        p_Step.get_renderpasses()[p_RenderFlow].end();
      }

      void setup_renderstep(GraphicsStep p_Step, RenderFlow p_RenderFlow)
      {
        Interface::RenderpassCreateParams l_Params;
        l_Params.context = p_Step.get_context();
        apply_dimensions_config(p_Step.get_context(), p_RenderFlow,
                                p_Step.get_config().get_dimensions_config(),
                                l_Params.dimensions);
        l_Params.useDepth = p_Step.get_config().is_use_depth();
        if (p_Step.get_config().is_depth_clear()) {
          l_Params.clearDepthColor = {1.0f, 1.0f};
        } else {
          l_Params.clearDepthColor = {1.0f, 0.0f};
        }

        for (uint8_t i = 0u; i < p_Step.get_config().get_rendertargets().size();
             ++i) {
          l_Params.clearColors.push_back({0.0f, 0.0f, 0.0f, 0.0f});

          if (p_Step.get_config().get_rendertargets()[i].resourceScope ==
              ResourceBindScope::RENDERFLOW) {
            Resource::Image i_Image =
                p_RenderFlow.get_resources().get_image_resource(
                    p_Step.get_config().get_rendertargets()[i].resourceName);
            LOW_ASSERT(i_Image.is_alive(),
                       "Could not find rendertarget image resource");

            l_Params.renderTargets.push_back(i_Image);
          } else {
            LOW_ASSERT(false, "Unsupported rendertarget resource scope");
          }
        }

        if (p_Step.get_config().is_use_depth()) {
          Resource::Image l_Image =
              p_RenderFlow.get_resources().get_image_resource(
                  p_Step.get_config().get_depth_rendertarget().resourceName);
          LOW_ASSERT(l_Image.is_alive(),
                     "Could not find rendertarget image resource");
          l_Params.depthRenderTarget = l_Image;
        }

        p_Step.get_renderpasses()[p_RenderFlow] =
            Interface::Renderpass::make(p_Step.get_name(), l_Params);
      }

      void setup_config()
      {
        GraphicsStepConfig l_Config =
            GraphicsStepConfig::make(N(DebugGeometryStep));
        l_Config.get_dimensions_config().type =
            ImageResourceDimensionType::RELATIVE;
        l_Config.get_dimensions_config().relative.target =
            ImageResourceDimensionRelativeOptions::RENDERFLOW;
        l_Config.get_dimensions_config().relative.multiplier = 1.0f;

        {
          l_Config.set_depth_clear(false);
          l_Config.set_depth_compare_operation(Backend::CompareOperation::LESS);
          l_Config.set_depth_test(true);
          l_Config.set_depth_write(true);
          l_Config.set_use_depth(true);

          {
            PipelineResourceBindingConfig l_ResourceBinding;
            l_ResourceBinding.resourceName = N(HoverHighlightImage);
            l_ResourceBinding.resourceScope = ResourceBindScope::RENDERFLOW;
            l_ResourceBinding.bindType = ResourceBindType::IMAGE;
            l_Config.get_rendertargets().push_back(l_ResourceBinding);
          }

          {
            PipelineResourceBindingConfig l_ResourceBinding;
            l_ResourceBinding.resourceName = N(GBufferDepth);
            l_ResourceBinding.resourceScope = ResourceBindScope::RENDERFLOW;
            l_ResourceBinding.bindType = ResourceBindType::IMAGE;
            l_Config.set_depth_rendertarget(l_ResourceBinding);
          }
        }

        {
          GraphicsPipelineConfig l_PipelineConfig =
              get_graphics_pipeline_config(N(debuggeometry));

          l_Config.get_pipelines().push_back(l_PipelineConfig);
        }

        l_Config.get_callbacks().setup_signature = &setup_signature;
        l_Config.get_callbacks().setup_pipelines =
            &GraphicsStep::create_pipelines;
        l_Config.get_callbacks().setup_renderpass = &setup_renderstep;
        l_Config.get_callbacks().execute = &execute;

        {
          ResourceConfig l_ResourceConfig;
          l_ResourceConfig.arraySize = 1;
          l_ResourceConfig.name = N(_renderobject_buffer);
          l_ResourceConfig.type = ResourceType::BUFFER;
          l_ResourceConfig.buffer.size = sizeof(RenderObjectShaderInfo) * 32u;
          l_Config.get_resources().push_back(l_ResourceConfig);
        }
        {
          ResourceConfig l_ResourceConfig;
          l_ResourceConfig.arraySize = 1;
          l_ResourceConfig.name = N(color_buffer);
          l_ResourceConfig.type = ResourceType::BUFFER;
          l_ResourceConfig.buffer.size = sizeof(Math::Vector4) * 32u;
          l_Config.get_resources().push_back(l_ResourceConfig);
        }
      }
    } // namespace DebugGeometryStep
  }   // namespace Renderer
} // namespace Low
