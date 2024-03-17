#include "LowRendererCustomRenderSteps.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "LowMathVectorUtil.h"
#include "LowMath.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "LowRendererGraphicsStepConfig.h"
#include "LowRendererInterface.h"
#include "LowRendererBackend.h"
#include "LowRendererGraphicsStep.h"
#include "LowRenderer.h"

namespace Low {
  namespace Renderer {
    namespace UiStep {
      // TODO: Hard coded rnederobjectcount for UI
      const u32 l_RenderObjectCount = 5000;

      void setup_signature(GraphicsStep p_Step,
                           RenderFlow p_RenderFlow)
      {
        Util::List<Backend::PipelineResourceDescription>
            l_ResourceDescriptions;

        {
          Backend::PipelineResourceDescription l_ResourceDescription;
          l_ResourceDescription.arraySize = 1;
          l_ResourceDescription.name = N(u_RenderObjects);
          l_ResourceDescription.step =
              Backend::ResourcePipelineStep::GRAPHICS;
          l_ResourceDescription.type = Backend::ResourceType::BUFFER;

          l_ResourceDescriptions.push_back(l_ResourceDescription);
        }
        {
          Backend::PipelineResourceDescription l_ResourceDescription;
          l_ResourceDescription.arraySize = 1;
          l_ResourceDescription.name = N(u_Colors);
          l_ResourceDescription.step =
              Backend::ResourcePipelineStep::GRAPHICS;
          l_ResourceDescription.type = Backend::ResourceType::BUFFER;

          l_ResourceDescriptions.push_back(l_ResourceDescription);
        }
        {
          Backend::PipelineResourceDescription l_ResourceDescription;
          l_ResourceDescription.arraySize = 1;
          l_ResourceDescription.name = N(u_ProjectionMatrix);
          l_ResourceDescription.step =
              Backend::ResourcePipelineStep::GRAPHICS;
          l_ResourceDescription.type = Backend::ResourceType::BUFFER;

          l_ResourceDescriptions.push_back(l_ResourceDescription);
        }
        p_Step.get_signatures()[p_RenderFlow] =
            Interface::PipelineResourceSignature::make(
                N(StepResourceSignature), p_Step.get_context(), 2,
                l_ResourceDescriptions);

        p_Step.get_signatures()[p_RenderFlow].set_buffer_resource(
            N(u_RenderObjects), 0,
            p_Step.get_resources()[p_RenderFlow].get_buffer_resource(
                N(_renderobject_buffer)));

        p_Step.get_signatures()[p_RenderFlow].set_buffer_resource(
            N(u_Colors), 0,
            p_Step.get_resources()[p_RenderFlow].get_buffer_resource(
                N(_color_buffer)));

        p_Step.get_signatures()[p_RenderFlow].set_buffer_resource(
            N(u_ProjectionMatrix), 0,
            p_Step.get_resources()[p_RenderFlow].get_buffer_resource(
                N(projection_matrix)));
      }

      void execute(GraphicsStep p_Step, RenderFlow p_RenderFlow,
                   Math::Matrix4x4 &p_ProjectionMatrix,
                   Math::Matrix4x4 &p_ViewMatrix)
      {
        float l_NearPlane = -100.0f;
        float l_FarPlane = 10.0f;

        Math::Vector2 l_Dimensions;
        l_Dimensions.x = (float)p_RenderFlow.get_dimensions().x;
        l_Dimensions.y = (float)p_RenderFlow.get_dimensions().y;

        Math::Matrix4x4 l_OrthographicMatrix =
            glm::ortho(0.0f, (float)p_RenderFlow.get_dimensions().x,
                       0.0f, (float)p_RenderFlow.get_dimensions().y,
                       l_NearPlane, l_FarPlane);

        p_Step.get_resources()[p_RenderFlow]
            .get_buffer_resource(N(projection_matrix))
            .set(&l_OrthographicMatrix);

        RenderObjectShaderInfo
            l_ObjectShaderInfos[l_RenderObjectCount];
        Math::Color l_Colors[l_RenderObjectCount];
        uint32_t l_ObjectIndex = 0;

        for (auto pit = p_Step.get_pipelines()[p_RenderFlow].begin();
             pit != p_Step.get_pipelines()[p_RenderFlow].end();
             ++pit) {
          Interface::GraphicsPipeline i_GraphicsPipeline = *pit;
          if (!i_GraphicsPipeline.is_alive()) {
            continue;
          }

          for (auto mit = p_Step
                              .get_renderobjects()[i_GraphicsPipeline
                                                       .get_name()]
                              .begin();
               mit !=
               p_Step
                   .get_renderobjects()[i_GraphicsPipeline.get_name()]
                   .end();
               ++mit) {
            for (auto it = mit->second.begin();
                 it != mit->second.end();) {
              RenderObject &i_RenderObject = *it;

              Math::Matrix4x4 l_MVPMatrix =
                  l_OrthographicMatrix * i_RenderObject.transform;

              l_Colors[l_ObjectIndex] = i_RenderObject.color;

              l_ObjectShaderInfos[l_ObjectIndex].mvp = l_MVPMatrix;
              l_ObjectShaderInfos[l_ObjectIndex].model_matrix =
                  i_RenderObject.transform;

              l_ObjectShaderInfos[l_ObjectIndex].material_index =
                  i_RenderObject.material.get_index();
              l_ObjectShaderInfos[l_ObjectIndex].entity_id =
                  i_RenderObject.entity_id;
              l_ObjectShaderInfos[l_ObjectIndex].texture_index =
                  i_RenderObject.texture.get_index();
              l_ObjectShaderInfos[l_ObjectIndex].click_passthrough =
                  i_RenderObject.clickPassthrough ? 1 : 0;

              l_ObjectIndex++;

              ++it;
            }
          }
        }

        p_Step.get_resources()[p_RenderFlow]
            .get_buffer_resource(N(_renderobject_buffer))
            .set((void *)l_ObjectShaderInfos);

        p_Step.get_resources()[p_RenderFlow]
            .get_buffer_resource(N(_color_buffer))
            .set((void *)l_Colors);

        p_Step.get_renderpasses()[p_RenderFlow].begin();
        GraphicsStep::draw_renderobjects(p_Step, p_RenderFlow);
        p_Step.get_renderpasses()[p_RenderFlow].end();
      }

      void setup_renderpass(GraphicsStep p_Step,
                            RenderFlow p_RenderFlow)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_create_renderpass
        Interface::RenderpassCreateParams l_Params;
        l_Params.context = p_Step.get_context();
        apply_dimensions_config(
            p_Step.get_context(), p_RenderFlow,
            p_Step.get_config().get_dimensions_config(),
            l_Params.dimensions);
        l_Params.useDepth = p_Step.get_config().is_use_depth();
        if (p_Step.get_config().is_depth_clear()) {
          l_Params.clearDepthColor = {1.0f, 1.0f};
        } else {
          l_Params.clearDepthColor = {1.0f, 0.0f};
        }

        for (uint8_t i = 0u;
             i < p_Step.get_config().get_rendertargets().size();
             ++i) {

          if (i == 1) {
            l_Params.clearColors.push_back(
                Math::Color(32000.0f, 0.0f, 0.0f, 1.0f));
          } else {
            l_Params.clearColors.push_back(
                p_Step.get_config().get_rendertargets_clearcolor());
          }

          if (p_Step.get_config()
                  .get_rendertargets()[i]
                  .resourceScope == ResourceBindScope::LOCAL) {
            Resource::Image i_Image =
                p_Step.get_resources()[p_RenderFlow]
                    .get_image_resource(p_Step.get_config()
                                            .get_rendertargets()[i]
                                            .resourceName);
            LOW_ASSERT(i_Image.is_alive(),
                       "Could not find rendertarget image resource");

            l_Params.renderTargets.push_back(i_Image);
          } else if (p_Step.get_config()
                         .get_rendertargets()[i]
                         .resourceScope ==
                     ResourceBindScope::RENDERFLOW) {
            Resource::Image i_Image =
                p_RenderFlow.get_resources().get_image_resource(
                    p_Step.get_config()
                        .get_rendertargets()[i]
                        .resourceName);
            LOW_ASSERT(i_Image.is_alive(),
                       "Could not find rendertarget image resource");

            l_Params.renderTargets.push_back(i_Image);
          } else {
            LOW_ASSERT(false,
                       "Unsupported rendertarget resource scope");
          }
        }

        if (p_Step.get_config().is_use_depth()) {
          Resource::Image l_Image =
              p_RenderFlow.get_resources().get_image_resource(
                  p_Step.get_config()
                      .get_depth_rendertarget()
                      .resourceName);
          LOW_ASSERT(l_Image.is_alive(),
                     "Could not find rendertarget image resource");
          l_Params.depthRenderTarget = l_Image;
        }

        p_Step.get_renderpasses()[p_RenderFlow] =
            Interface::Renderpass::make(p_Step.get_name(), l_Params);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_create_renderpass
      }

      void setup_config()
      {
        GraphicsStepConfig l_Config =
            GraphicsStepConfig::make(N(UiPass));
        l_Config.get_dimensions_config().type =
            ImageResourceDimensionType::RELATIVE;
        l_Config.get_dimensions_config().relative.multiplier = 1.0f;
        l_Config.get_dimensions_config().relative.target =
            ImageResourceDimensionRelativeOptions::RENDERFLOW;

        l_Config.set_rendertargets_clearcolor(
            Math::Color(0.0f, 0.0f, 0.0f, 0.0f));

        {
          l_Config.set_depth_clear(true);
          l_Config.set_depth_compare_operation(
              Backend::CompareOperation::LESS);
          l_Config.set_depth_test(true);
          l_Config.set_depth_write(true);
          l_Config.set_use_depth(true);

          PipelineResourceBindingConfig l_ResourceBinding;
          l_ResourceBinding.resourceName = N(UiDepth);
          l_ResourceBinding.resourceScope =
              ResourceBindScope::RENDERFLOW;
          l_ResourceBinding.bindType = ResourceBindType::IMAGE;
          l_Config.set_depth_rendertarget(l_ResourceBinding);
        }

        {
          PipelineResourceBindingConfig l_ResourceBinding;
          l_ResourceBinding.resourceName = N(HoverHighlightImage);
          l_ResourceBinding.resourceScope =
              ResourceBindScope::RENDERFLOW;
          l_ResourceBinding.bindType = ResourceBindType::IMAGE;
          l_Config.get_rendertargets().push_back(l_ResourceBinding);
        }
        {
          PipelineResourceBindingConfig l_ResourceBinding;
          l_ResourceBinding.resourceName = N(ElementIndexMap);
          l_ResourceBinding.resourceScope =
              ResourceBindScope::RENDERFLOW;
          l_ResourceBinding.bindType = ResourceBindType::IMAGE;
          l_Config.get_rendertargets().push_back(l_ResourceBinding);
        }

        {
          GraphicsPipelineConfig l_PipelineConfig =
              get_graphics_pipeline_config(N(ui_flat));

          l_Config.get_pipelines().push_back(l_PipelineConfig);
        }
        {
          GraphicsPipelineConfig l_PipelineConfig =
              get_graphics_pipeline_config(N(ui_text));

          l_Config.get_pipelines().push_back(l_PipelineConfig);
        }

        l_Config.get_callbacks().setup_signature = &setup_signature;
        l_Config.get_callbacks().setup_pipelines =
            &GraphicsStep::create_pipelines;
        l_Config.get_callbacks().setup_renderpass = &setup_renderpass;
        l_Config.get_callbacks().execute = &execute;

        {
          ResourceConfig l_ResourceConfig;
          l_ResourceConfig.arraySize = 1;
          l_ResourceConfig.name = N(_renderobject_buffer);
          l_ResourceConfig.type = ResourceType::BUFFER;
          l_ResourceConfig.buffer.size =
              sizeof(RenderObjectShaderInfo) * l_RenderObjectCount;
          l_Config.get_resources().push_back(l_ResourceConfig);
        }

        {
          ResourceConfig l_ResourceConfig;
          l_ResourceConfig.arraySize = 1;
          l_ResourceConfig.name = N(_color_buffer);
          l_ResourceConfig.type = ResourceType::BUFFER;
          l_ResourceConfig.buffer.size =
              sizeof(Math::Color) * l_RenderObjectCount;
          l_Config.get_resources().push_back(l_ResourceConfig);
        }

        {
          ResourceConfig l_ResourceConfig;
          l_ResourceConfig.arraySize = 1;
          l_ResourceConfig.name = N(projection_matrix);
          l_ResourceConfig.type = ResourceType::BUFFER;
          l_ResourceConfig.buffer.size = sizeof(Math::Matrix4x4);
          l_Config.get_resources().push_back(l_ResourceConfig);
        }

        {
          ResourceConfig l_ResourceConfig;
          l_ResourceConfig.arraySize = 1;
          l_ResourceConfig.name = N(out_image);
          l_ResourceConfig.type = ResourceType::IMAGE;
          l_ResourceConfig.image.depth = false;
          l_ResourceConfig.image.format =
              Backend::ImageFormat::RGBA8_UNORM;
          l_ResourceConfig.image.dimensions.type =
              ImageResourceDimensionType::RELATIVE;
          l_ResourceConfig.image.dimensions.relative.target =
              ImageResourceDimensionRelativeOptions::RENDERFLOW;
          l_ResourceConfig.image.dimensions.relative.multiplier =
              1.0f;
          l_ResourceConfig.image.sampleFilter =
              Backend::ImageSampleFilter::LINEAR;
          l_Config.get_resources().push_back(l_ResourceConfig);
        }

        {
          PipelineResourceBindingConfig l_OutConfig;
          l_OutConfig.bindType = ResourceBindType::IMAGE;
          l_OutConfig.resourceName = N(INPUT_IMAGE);
          l_OutConfig.resourceScope = ResourceBindScope::LOCAL;

          l_Config.set_output_image(l_OutConfig);
        }
      }
    } // namespace UiStep
  }   // namespace Renderer
} // namespace Low
