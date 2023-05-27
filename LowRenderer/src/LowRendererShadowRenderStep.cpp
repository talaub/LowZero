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

      void get_frustum_corners_world_space(const glm::mat4 &p_Projection,
                                           const glm::mat4 &p_View,
                                           Math::Vector4 *p_OutPositions)
      {
        const auto inv = glm::inverse(p_Projection * p_View);

        uint32_t l_Index = 0;

        for (unsigned int x = 0; x < 2; ++x) {
          for (unsigned int y = 0; y < 2; ++y) {
            for (unsigned int z = 0; z < 2; ++z) {
              const glm::vec4 pt =
                  inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f,
                                  2.0f * z - 1.0f, 1.0f);
              p_OutPositions[l_Index++] = pt / pt.w;
            }
          }
        }
      }

      static void
      calculate_directional_light_matrices(RenderFlow p_RenderFlow,
                                           Math::Matrix4x4 *p_LightProjection,
                                           Math::Matrix4x4 *p_LightView)
      {

#define FRUSTUM_POINT_COUNT 8

        Math::Matrix4x4 proj =
            glm::perspective(glm::radians(p_RenderFlow.get_camera_fov()),
                             ((float)p_RenderFlow.get_dimensions().x) /
                                 ((float)p_RenderFlow.get_dimensions().y),
                             p_RenderFlow.get_camera_near_plane(), 15.0f);

        proj[1][1] *= -1.0f; // Convert from OpenGL y-axis
        //    to Vulkan y-axis

        Math::Matrix4x4 viewMatrix =
            glm::lookAt(p_RenderFlow.get_camera_position(),
                        p_RenderFlow.get_camera_position() +
                            p_RenderFlow.get_camera_direction(),
                        LOW_VECTOR3_UP);

        DirectionalLight &l_DirectionalLight =
            p_RenderFlow.get_directional_light();

        glm::vec3 lightDirection = glm::normalize(Math::VectorUtil::direction(
                                       l_DirectionalLight.rotation)) *
                                   -1.0f;

        Math::Vector4 l_Corners[FRUSTUM_POINT_COUNT];
        get_frustum_corners_world_space(proj, viewMatrix, l_Corners);

        glm::vec3 center = glm::vec3(0, 0, 0);
        for (uint32_t i = 0u; i < FRUSTUM_POINT_COUNT; ++i) {
          center += glm::vec3(l_Corners[i]);
        }
        center /= FRUSTUM_POINT_COUNT;

        const auto lightView = glm::lookAt(center + lightDirection, center,
                                           glm::vec3(0.0f, 1.0f, 0.0f));

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();
        for (uint32_t i = 0u; i < FRUSTUM_POINT_COUNT; ++i) {
          const auto trf = lightView * l_Corners[i];
          minX = std::min(minX, trf.x);
          maxX = std::max(maxX, trf.x);
          minY = std::min(minY, trf.y);
          maxY = std::max(maxY, trf.y);
          minZ = std::min(minZ, trf.z);
          maxZ = std::max(maxZ, trf.z);
        }

        // Tune this parameter according to the scene
        constexpr float zMult = 2.0f;
        if (minZ < 0) {
          minZ *= zMult;
        } else {
          minZ /= zMult;
        }
        if (maxZ < 0) {
          maxZ /= zMult;
        } else {
          maxZ *= zMult;
        }

        const glm::mat4 lightProjection =
            glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

        *p_LightView = lightView;
        *p_LightProjection = lightProjection;

#undef FRUSTUM_POINT_COUNT
      }

      void execute(GraphicsStep p_Step, RenderFlow p_RenderFlow,
                   Math::Matrix4x4 &p_ProjectionMatrix,
                   Math::Matrix4x4 &p_ViewMatrix)
      {
        DirectionalLight &l_DirectionalLight =
            p_RenderFlow.get_directional_light();

        Math::Vector3 l_LightDirection =
            Math::VectorUtil::direction(l_DirectionalLight.rotation);

        Math::Matrix4x4 l_ProjectionMatrix;
        Math::Matrix4x4 l_ViewMatrix;
        calculate_directional_light_matrices(p_RenderFlow, &l_ProjectionMatrix,
                                             &l_ViewMatrix);

        // l_ProjectionMatrix[1][1] *= -1.0f; // Convert from OpenGL y-axis

        Math::Matrix4x4 l_LightSpace = l_ProjectionMatrix * l_ViewMatrix;

        DirectionalLightShaderInfo l_DirectionalLightShaderInfo;
        l_DirectionalLightShaderInfo.lightSpaceMatrix = l_LightSpace;
        l_DirectionalLightShaderInfo.atlasBounds =
            Math::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
        l_DirectionalLightShaderInfo.direction = l_LightDirection;
        l_DirectionalLightShaderInfo.color =
            p_RenderFlow.get_directional_light().color;

        p_RenderFlow.get_resources()
            .get_buffer_resource(N(_directional_light_info))
            .set(&l_DirectionalLightShaderInfo);

        {
          PointLightShaderInfo l_PointLights[LOW_RENDERER_POINTLIGHT_COUNT];
          uint32_t l_PointLightCount = p_RenderFlow.get_point_lights().size();
          l_PointLightCount = Math::Util::clamp(l_PointLightCount, 0,
                                                LOW_RENDERER_POINTLIGHT_COUNT);

          p_RenderFlow.get_resources()
              .get_buffer_resource(N(_point_light_info))
              .write(&l_PointLightCount, sizeof(uint32_t), 0);

          for (uint32_t i = 0u; i < l_PointLightCount; ++i) {
            l_PointLights[i].color = p_RenderFlow.get_point_lights()[i].color;
            l_PointLights[i].position =
                p_RenderFlow.get_point_lights()[i].position;
          }

          p_RenderFlow.get_resources()
              .get_buffer_resource(N(_point_light_info))
              .write(l_PointLights,
                     sizeof(PointLightShaderInfo) *
                         LOW_RENDERER_POINTLIGHT_COUNT,
                     16);

          p_RenderFlow.get_point_lights().clear();
        }

        RenderObjectShaderInfo l_ObjectShaderInfos[32];
        uint32_t l_ObjectIndex = 0;

        for (auto pit = p_Step.get_pipelines()[p_RenderFlow].begin();
             pit != p_Step.get_pipelines()[p_RenderFlow].end(); ++pit) {

          for (auto mit =
                   p_Step.get_skinned_renderobjects()[pit->get_name()].begin();
               mit != p_Step.get_skinned_renderobjects()[pit->get_name()].end();
               ++mit) {
            RenderObject &i_RenderObject = *mit;

            Math::Matrix4x4 l_MVPMatrix =
                l_ProjectionMatrix * l_ViewMatrix * i_RenderObject.transform;

            l_ObjectShaderInfos[l_ObjectIndex].mvp = l_MVPMatrix;

            l_ObjectIndex++;
          }

          for (auto mit = p_Step.get_renderobjects()[pit->get_name()].begin();
               mit != p_Step.get_renderobjects()[pit->get_name()].end();
               ++mit) {
            for (auto it = mit->second.begin(); it != mit->second.end();) {
              Math::Matrix4x4 l_MVPMatrix =
                  l_ProjectionMatrix * l_ViewMatrix * it->transform;

              l_ObjectShaderInfos[l_ObjectIndex].mvp = l_MVPMatrix;

              l_ObjectIndex++;

              ++it;
            }
          }
        }

        p_Step.get_resources()[p_RenderFlow]
            .get_buffer_resource(N(_renderobject_buffer))
            .set((void *)l_ObjectShaderInfos);

        p_Step.get_renderpasses()[p_RenderFlow].begin();
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
