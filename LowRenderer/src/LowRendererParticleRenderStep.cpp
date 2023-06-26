#include "LowRendererCustomRenderSteps.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include <random>

#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "LowMathVectorUtil.h"

#include "LowRendererComputeStepConfig.h"
#include "LowRendererGraphicsStepConfig.h"
#include "LowRendererInterface.h"
#include "LowRendererBackend.h"
#include "LowRendererComputeStep.h"
#include "LowRendererGraphicsStep.h"
#include "LowRenderer.h"

namespace Low {
  namespace Renderer {
    namespace ParticlePrepareStep {
      static void setup_signatures(ComputeStep p_Step, RenderFlow p_RenderFlow)
      {
        Util::List<Backend::PipelineResourceDescription> l_ResourceDescriptions;
        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_ParticleEmitterBuffer);
          l_Resource.arraySize = 1;
          l_Resource.step = Backend::ResourcePipelineStep::COMPUTE;
          l_Resource.type = Backend::ResourceType::BUFFER;
          l_ResourceDescriptions.push_back(l_Resource);
        }
        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_ParticleBuffer);
          l_Resource.arraySize = 1;
          l_Resource.step = Backend::ResourcePipelineStep::COMPUTE;
          l_Resource.type = Backend::ResourceType::BUFFER;
          l_ResourceDescriptions.push_back(l_Resource);
        }
        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_ParticleDrawBuffer);
          l_Resource.arraySize = 1;
          l_Resource.step = Backend::ResourcePipelineStep::COMPUTE;
          l_Resource.type = Backend::ResourceType::BUFFER;
          l_ResourceDescriptions.push_back(l_Resource);
        }
        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_ParticleRenderBuffer);
          l_Resource.arraySize = 1;
          l_Resource.step = Backend::ResourcePipelineStep::COMPUTE;
          l_Resource.type = Backend::ResourceType::BUFFER;
          l_ResourceDescriptions.push_back(l_Resource);
        }
        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_ParticleDrawCountBuffer);
          l_Resource.arraySize = 1;
          l_Resource.step = Backend::ResourcePipelineStep::COMPUTE;
          l_Resource.type = Backend::ResourceType::BUFFER;
          l_ResourceDescriptions.push_back(l_Resource);
        }

        p_Step.get_signatures()[p_RenderFlow].push_back(
            Interface::PipelineResourceSignature::make(p_Step.get_name(),
                                                       p_Step.get_context(), 2,
                                                       l_ResourceDescriptions));
      }

      static void populate_signatures(ComputeStep p_Step,
                                      RenderFlow p_RenderFlow)
      {
        Interface::PipelineResourceSignature l_Signature =
            p_Step.get_signatures()[p_RenderFlow][0];

        l_Signature.set_buffer_resource(N(u_ParticleEmitterBuffer), 0,
                                        get_particle_emitter_buffer());
        l_Signature.set_buffer_resource(N(u_ParticleBuffer), 0,
                                        get_particle_buffer());

        l_Signature.set_buffer_resource(
            N(u_ParticleDrawBuffer), 0,
            p_RenderFlow.get_resources().get_buffer_resource(
                N(_particle_draw_info)));

        l_Signature.set_buffer_resource(
            N(u_ParticleRenderBuffer), 0,
            p_RenderFlow.get_resources().get_buffer_resource(
                N(_particle_render_info)));

        l_Signature.set_buffer_resource(
            N(u_ParticleDrawCountBuffer), 0,
            p_RenderFlow.get_resources().get_buffer_resource(
                N(_particle_draw_count)));
      }

      static void execute(ComputeStep p_Step, RenderFlow p_RenderFlow)
      {
        uint32_t l_Val = 0;
        p_RenderFlow.get_resources()
            .get_buffer_resource(N(_particle_draw_count))
            .set(&l_Val);

        Util::List<uint8_t> l_DrawData;
        l_DrawData.resize(
            Backend::callbacks().get_draw_indexed_indirect_info_size() *
            LOW_RENDERER_MAX_PARTICLES);
        p_RenderFlow.get_resources()
            .get_buffer_resource(N(_particle_draw_info))
            .set(l_DrawData.data());

        ComputeStep::default_execute(p_Step, p_RenderFlow);
      }

      void setup_config()
      {
        ComputeStepConfig l_Config =
            ComputeStepConfig::make(N(ParticlePrepare));

        l_Config.get_callbacks().setup_pipelines =
            &ComputeStep::create_pipelines;
        l_Config.get_callbacks().setup_signatures = &setup_signatures;
        l_Config.get_callbacks().populate_signatures = &populate_signatures;
        l_Config.get_callbacks().execute = &execute;

        {
          ComputePipelineConfig l_PipelineConfig;
          l_PipelineConfig.name = N(Particle Preparation);
          l_PipelineConfig.shader = "particle_preparation.comp";
          l_PipelineConfig.dispatchConfig.dimensionType =
              ComputeDispatchDimensionType::ABSOLUTE;
          l_PipelineConfig.dispatchConfig.absolute.x =
              (LOW_RENDERER_MAX_PARTICLES / 256) + 1;
          l_PipelineConfig.dispatchConfig.absolute.y = 1;
          l_PipelineConfig.dispatchConfig.absolute.z = 1;

          /*
                {
                  PipelineResourceBindingConfig l_ResourceConfig;
                  l_ResourceConfig.resourceName = N(u_Particle);
                  l_ResourceConfig.bindType = ResourceBindType::BUFFER;
                  l_ResourceConfig.resourceScope = ResourceBindScope::LOCAL;
                  l_PipelineConfig.resourceBinding.push_back(l_ResourceConfig);
                }
          */

          l_Config.get_pipelines().push_back(l_PipelineConfig);
        }
      }
    } // namespace ParticlePrepareStep

    namespace ParticleRenderStep {
      static void execute(GraphicsStep p_Step, RenderFlow p_RenderFlow,
                          Math::Matrix4x4 &p_ProjectionMatrix,
                          Math::Matrix4x4 &p_ViewMatrix)
      {
        p_Step.get_renderpasses()[p_RenderFlow].begin();
        p_Step.get_pipelines()[p_RenderFlow][0].bind();
        get_vertex_buffer().bind_vertex();
        Backend::callbacks().draw_indexed_indirect_count(
            p_Step.get_context().get_context(),
            p_RenderFlow.get_resources()
                .get_buffer_resource(N(_particle_draw_info))
                .get_buffer(),
            0,
            p_RenderFlow.get_resources()
                .get_buffer_resource(N(_particle_draw_count))
                .get_buffer(),
            0, LOW_RENDERER_MAX_PARTICLES,
            Backend::callbacks().get_draw_indexed_indirect_info_size());
        p_Step.get_renderpasses()[p_RenderFlow].end();
      }

      void setup_signature(GraphicsStep p_Step, RenderFlow p_RenderFlow)
      {
        Util::List<Backend::PipelineResourceDescription> l_ResourceDescriptions;

        {
          Backend::PipelineResourceDescription l_ResourceDescription;
          l_ResourceDescription.arraySize = 1;
          l_ResourceDescription.name = N(u_Particles);
          l_ResourceDescription.step = Backend::ResourcePipelineStep::GRAPHICS;
          l_ResourceDescription.type = Backend::ResourceType::BUFFER;

          l_ResourceDescriptions.push_back(l_ResourceDescription);
        }

        p_Step.get_signatures()[p_RenderFlow] =
            Interface::PipelineResourceSignature::make(N(StepResourceSignature),
                                                       p_Step.get_context(), 2,
                                                       l_ResourceDescriptions);

        p_Step.get_signatures()[p_RenderFlow].set_buffer_resource(
            N(u_Particles), 0,
            p_RenderFlow.get_resources().get_buffer_resource(
                N(_particle_render_info)));
      }

      void setup_config()
      {
        GraphicsStepConfig l_Config = GraphicsStepConfig::make(N(Particle));
        l_Config.get_dimensions_config().type =
            ImageResourceDimensionType::RELATIVE;
        l_Config.get_dimensions_config().relative.target =
            ImageResourceDimensionRelativeOptions::RENDERFLOW;
        l_Config.get_dimensions_config().relative.multiplier = 1.0f;

        l_Config.get_callbacks().setup_signature = &setup_signature;
        l_Config.get_callbacks().setup_pipelines =
            &GraphicsStep::create_pipelines;
        l_Config.get_callbacks().setup_renderpass =
            &GraphicsStep::create_renderpass;
        l_Config.get_callbacks().execute = &execute;

        l_Config.set_rendertargets_clearcolor(
            Math::Color(0.0f, 0.0f, 0.0f, 0.0f));

        {
          PipelineResourceBindingConfig l_ResourceBinding;
          parse_pipeline_resource_binding(
              l_ResourceBinding, Util::String("renderflow:DeferredLit"),
              Util::String("sampler"));

          l_Config.get_rendertargets().push_back(l_ResourceBinding);
        }

        {
          GraphicsPipelineConfig l_PipelineConfig =
              get_graphics_pipeline_config(N(particles));
          l_Config.get_pipelines().push_back(l_PipelineConfig);
        }

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
      }
    } // namespace ParticleRenderStep
  }   // namespace Renderer
} // namespace Low
