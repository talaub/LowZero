#include "LowRendererCustomRenderSteps.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include "LowMathVectorUtil.h"

#include "LowRendererGraphicsStepConfig.h"
#include "LowRendererInterface.h"
#include "LowRendererBackend.h"
#include "LowRendererGraphicsStep.h"

namespace Low {
  namespace Renderer {
    namespace ShadowStep {
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
        p_Step.get_signatures()[p_RenderFlow] =
            Interface::PipelineResourceSignature::make(N(StepResourceSignature),
                                                       p_Step.get_context(), 2,
                                                       l_ResourceDescriptions);

        p_Step.get_signatures()[p_RenderFlow].set_buffer_resource(
            N(u_RenderObjects), 0,
            p_Step.get_resources()[p_RenderFlow].get_buffer_resource(
                N(_renderobject_buffer)));
      }

      void execute(GraphicsStep p_Step, RenderFlow p_RenderFlow,
                   Math::Matrix4x4 &p_ProjectionMatrix,
                   Math::Matrix4x4 &p_ViewMatrix)
      {
        DirectionalLight &l_DirectionalLight =
            p_RenderFlow.get_directional_light();

        float near_plane = 0.1f, far_plane = 70.0f;
        Math::Matrix4x4 l_ProjectionMatrix =
            glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
        l_ProjectionMatrix[1][1] *= -1.0f; // Convert from OpenGL y-axis to
                                           // Vulkan y-axis
        l_ProjectionMatrix[0][0] *= -1.0f; // Convert to left handed system

        Math::Vector3 l_DirectionalLightPosition =
            Math::Vector3(0.0f, 0.0f, -5.0f);
        l_DirectionalLightPosition += (l_DirectionalLight.direction * -13.0f);

        Math::Matrix4x4 l_ViewMatrix = glm::lookAt(
            l_DirectionalLightPosition,
            l_DirectionalLightPosition + l_DirectionalLight.direction,
            Math::Vector3(0.0f, 1.0f, 0.0f));

        Math::Matrix4x4 l_LightSpace = l_ProjectionMatrix * l_ViewMatrix;

        DirectionalLightShaderInfo l_DirectionalLightShaderInfo;
        l_DirectionalLightShaderInfo.lightSpaceMatrix = l_LightSpace;
        l_DirectionalLightShaderInfo.atlasBounds =
            Math::Vector4(0.0f, 0.0f, 1.0f, 1.0f);

        p_RenderFlow.get_resources()
            .get_buffer_resource(N(_directional_light_info))
            .set(&l_DirectionalLightShaderInfo);

        p_Step.get_renderpasses()[p_RenderFlow].begin();

        RenderObjectShaderInfo l_ObjectShaderInfos[32];
        uint32_t l_ObjectIndex = 0;

        for (auto pit = p_Step.get_pipelines()[p_RenderFlow].begin();
             pit != p_Step.get_pipelines()[p_RenderFlow].end(); ++pit) {
          Interface::GraphicsPipeline i_Pipeline = *pit;
          i_Pipeline.bind();

          for (auto mit = p_Step.get_renderobjects()[pit->get_name()].begin();
               mit != p_Step.get_renderobjects()[pit->get_name()].end();
               ++mit) {
            for (auto it = mit->second.begin(); it != mit->second.end();) {
              RenderObject i_RenderObject = *it;

              if (!i_RenderObject.is_alive()) {
                it = mit->second.erase(it);
                continue;
              }

              Math::Matrix4x4 l_ModelMatrix =
                  glm::translate(glm::mat4(1.0f),
                                 i_RenderObject.get_world_position()) *
                  glm::toMat4(i_RenderObject.get_world_rotation()) *
                  glm::scale(glm::mat4(1.0f), i_RenderObject.get_world_scale());

              Math::Matrix4x4 l_MVPMatrix =
                  l_ProjectionMatrix * l_ViewMatrix * l_ModelMatrix;

              l_ObjectShaderInfos[l_ObjectIndex].mvp = l_MVPMatrix;

              l_ObjectIndex++;

              ++it;
            }
          }
        }

        p_Step.get_resources()[p_RenderFlow]
            .get_buffer_resource(N(_renderobject_buffer))
            .set((void *)l_ObjectShaderInfos);

        GraphicsStep::draw_renderobjects(p_Step, p_RenderFlow);

        p_Step.get_renderpasses()[p_RenderFlow].end();
      }

      void setup_config()
      {
        GraphicsStepConfig l_Config = GraphicsStepConfig::make(N(ShadowPass));
        l_Config.get_dimensions_config().type =
            ImageResourceDimensionType::ABSOLUTE;
        l_Config.get_dimensions_config().absolute.x = 1024;
        l_Config.get_dimensions_config().absolute.y = 1024;

        {
          l_Config.set_depth_clear(true);
          l_Config.set_depth_compare_operation(Backend::CompareOperation::LESS);
          l_Config.set_depth_test(true);
          l_Config.set_depth_write(true);
          l_Config.set_use_depth(true);

          PipelineResourceBindingConfig l_ResourceBinding;
          l_ResourceBinding.resourceName = N(ShadowAtlas);
          l_ResourceBinding.resourceScope = ResourceBindScope::RENDERFLOW;
          l_ResourceBinding.bindType = ResourceBindType::IMAGE;
          l_Config.set_depth_rendertarget(l_ResourceBinding);
        }

        {
          GraphicsPipelineConfig l_PipelineConfig =
              get_graphics_pipeline_config(N(testgp_depth));

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

        l_Config.get_callbacks().setup_signature = &setup_signature;
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
          l_ResourceConfig.buffer.size = sizeof(RenderObjectShaderInfo) * 32u;
          l_Config.get_resources().push_back(l_ResourceConfig);
        }
      }
    } // namespace ShadowStep
  }   // namespace Renderer
} // namespace Low
