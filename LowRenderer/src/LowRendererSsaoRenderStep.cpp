#include "LowRendererCustomRenderSteps.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include <random>

#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "LowMathVectorUtil.h"

#include "LowRendererComputeStepConfig.h"
#include "LowRendererInterface.h"
#include "LowRendererBackend.h"
#include "LowRendererComputeStep.h"
#include "LowRenderer.h"

namespace Low {
  namespace Renderer {
    namespace SsaoStep {
      struct KernelInput
      {
        alignas(16) glm::vec3 vec;
      };

      Texture2D g_NoiseTexture;

      float lerp(float a, float b, float f)
      {
        return a + f * (b - a);
      }

      void setup_signature(ComputeStep p_Step, RenderFlow p_RenderFlow)
      {
        ComputeStep::create_signatures(p_Step, p_RenderFlow);
        {
          std::uniform_real_distribution<float> randomFloats(
              0.0, 1.0); // generates random floats between 0.0 and 1.0
          std::default_random_engine generator;
          std::vector<KernelInput> ssaoKernel;
          for (unsigned int i = 0; i < 64; ++i) {
            glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0,
                             randomFloats(generator) * 2.0 - 1.0,
                             randomFloats(generator));
            sample = glm::normalize(sample);
            sample *= randomFloats(generator);
            float scale = float(i) / 64.0f;

            // scale samples s.t. they're more aligned to center of kernel
            scale = lerp(0.1f, 1.0f, scale * scale);
            sample *= scale;
            KernelInput i_Input;
            i_Input.vec = sample;
            ssaoKernel.push_back(i_Input);
          }

          p_Step.get_resources()[p_RenderFlow]
              .get_buffer_resource(N(kernel))
              .set(ssaoKernel.data());

          int l_NoiseTextureId = g_NoiseTexture.get_index();
          p_Step.get_resources()[p_RenderFlow]
              .get_buffer_resource(N(noise_texture_id))
              .set(&l_NoiseTextureId);
        }
      }

      void setup_config()
      {
        ComputeStepConfig l_Config = ComputeStepConfig::make(N(SsaoPass));

        l_Config.get_callbacks().setup_pipelines =
            &ComputeStep::create_pipelines;
        l_Config.get_callbacks().setup_signatures = &setup_signature;
        l_Config.get_callbacks().populate_signatures =
            &ComputeStep::prepare_signatures;
        l_Config.get_callbacks().execute = &ComputeStep::default_execute;

        std::uniform_real_distribution<float> randomFloats(
            0.0, 1.0); // generates random floats between 0.0 and 1.0
        std::default_random_engine generator;

        Util::Resource::Image2D l_Image;
        l_Image.dimensions.x = 4;
        l_Image.dimensions.y = 4;
        std::vector<glm::vec4> ssaoNoise;
        for (unsigned int i = 0; i < 16; i++) {
          glm::vec4 noise(randomFloats(generator), randomFloats(generator),
                          0.0f,
                          1.0f); // rotate around z-axis (in tangent space)
          l_Image.data.push_back(noise.x * 255.0f);
          l_Image.data.push_back(noise.y * 255.0f);
          l_Image.data.push_back(0);
          l_Image.data.push_back(255);
        }
        g_NoiseTexture = upload_texture(N(BasicNoise), l_Image);

        {
          ResourceConfig l_ResourceConfig;
          l_ResourceConfig.arraySize = 1;
          l_ResourceConfig.name = N(kernel);
          l_ResourceConfig.type = ResourceType::BUFFER;
          l_ResourceConfig.buffer.size = 64u * sizeof(KernelInput);
          l_Config.get_resources().push_back(l_ResourceConfig);
        }
        {
          ResourceConfig l_ResourceConfig;
          l_ResourceConfig.arraySize = 1;
          l_ResourceConfig.name = N(noise_texture_id);
          l_ResourceConfig.type = ResourceType::BUFFER;
          l_ResourceConfig.buffer.size = sizeof(int);
          l_Config.get_resources().push_back(l_ResourceConfig);
        }

        {
          ComputePipelineConfig l_PipelineConfig;
          l_PipelineConfig.name = N(Ssao Calculation);
          l_PipelineConfig.shader = "ssao.comp";
          l_PipelineConfig.dispatchConfig.dimensionType =
              ComputeDispatchDimensionType::RELATIVE;
          l_PipelineConfig.dispatchConfig.relative.multiplier = 0.065f;
          l_PipelineConfig.dispatchConfig.relative.target =
              ComputeDispatchRelativeTarget::RENDERFLOW;

          {
            PipelineResourceBindingConfig l_ResourceConfig;
            l_ResourceConfig.resourceName = N(SsaoMask);
            l_ResourceConfig.bindType = ResourceBindType::IMAGE;
            l_ResourceConfig.resourceScope = ResourceBindScope::RENDERFLOW;
            l_PipelineConfig.resourceBinding.push_back(l_ResourceConfig);
          }
          {
            PipelineResourceBindingConfig l_ResourceConfig;
            l_ResourceConfig.resourceName = N(GBufferDepth);
            l_ResourceConfig.bindType = ResourceBindType::SAMPLER;
            l_ResourceConfig.resourceScope = ResourceBindScope::RENDERFLOW;
            l_PipelineConfig.resourceBinding.push_back(l_ResourceConfig);
          }
          {
            PipelineResourceBindingConfig l_ResourceConfig;
            l_ResourceConfig.resourceName = N(GBufferNormals);
            l_ResourceConfig.bindType = ResourceBindType::SAMPLER;
            l_ResourceConfig.resourceScope = ResourceBindScope::RENDERFLOW;
            l_PipelineConfig.resourceBinding.push_back(l_ResourceConfig);
          }
          {
            PipelineResourceBindingConfig l_ResourceConfig;
            l_ResourceConfig.resourceName = N(GBufferSurfaceNormals);
            l_ResourceConfig.bindType = ResourceBindType::SAMPLER;
            l_ResourceConfig.resourceScope = ResourceBindScope::RENDERFLOW;
            l_PipelineConfig.resourceBinding.push_back(l_ResourceConfig);
          }
          {
            PipelineResourceBindingConfig l_ResourceConfig;
            l_ResourceConfig.resourceName = N(noise_texture_id);
            l_ResourceConfig.bindType = ResourceBindType::BUFFER;
            l_ResourceConfig.resourceScope = ResourceBindScope::LOCAL;
            l_PipelineConfig.resourceBinding.push_back(l_ResourceConfig);
          }
          {
            PipelineResourceBindingConfig l_ResourceConfig;
            l_ResourceConfig.resourceName = N(kernel);
            l_ResourceConfig.bindType = ResourceBindType::BUFFER;
            l_ResourceConfig.resourceScope = ResourceBindScope::LOCAL;
            l_PipelineConfig.resourceBinding.push_back(l_ResourceConfig);
          }

          l_Config.get_pipelines().push_back(l_PipelineConfig);
        }

        {
          ComputePipelineConfig l_PipelineConfig;
          l_PipelineConfig.name = N(Ssao Blur);
          l_PipelineConfig.shader = "blur.comp";
          l_PipelineConfig.dispatchConfig.dimensionType =
              ComputeDispatchDimensionType::RELATIVE;
          l_PipelineConfig.dispatchConfig.relative.multiplier = 0.0325f;
          l_PipelineConfig.dispatchConfig.relative.target =
              ComputeDispatchRelativeTarget::RENDERFLOW;

          {
            PipelineResourceBindingConfig l_ResourceConfig;
            l_ResourceConfig.resourceName = N(SsaoMask);
            l_ResourceConfig.bindType = ResourceBindType::SAMPLER;
            l_ResourceConfig.resourceScope = ResourceBindScope::RENDERFLOW;
            l_PipelineConfig.resourceBinding.push_back(l_ResourceConfig);
          }
          {
            PipelineResourceBindingConfig l_ResourceConfig;
            l_ResourceConfig.resourceName = N(BlurredSsaoMask);
            l_ResourceConfig.bindType = ResourceBindType::IMAGE;
            l_ResourceConfig.resourceScope = ResourceBindScope::RENDERFLOW;
            l_PipelineConfig.resourceBinding.push_back(l_ResourceConfig);
          }

          l_Config.get_pipelines().push_back(l_PipelineConfig);
        }
      }
    } // namespace SsaoStep
  }   // namespace Renderer
} // namespace Low
