#pragma once

#include "LowRendererMesh.h"
#include "LowRendererSkeleton.h"
#include "LowRendererSkeletalAnimation.h"
#include "LowRendererMaterial.h"
#include "LowRendererTexture2D.h"

#include "LowMath.h"

#define LOW_RENDERER_MAX_PARTICLES 512

namespace Low {
  namespace Renderer {
    struct RenderObject
    {
      Math::Matrix4x4 transform;

      Mesh mesh;
      Material material;
      Texture2D texture;

      Math::Color color;

      uint64_t entity_id;

      bool useSkinningBuffer = false;
      uint32_t vertexBufferStartOverride = 0;
    };

    struct ParticleEmitter
    {
      alignas(16) Math::Vector3 position;
      alignas(16) Math::Vector3 positionOffset;
      float minLifetime;
      float maxLifetime;
      uint32_t maxParticles;
      uint32_t particleStart;
      uint32_t vertexStart;
      uint32_t indexStart;
      uint32_t indexCount;
    };

    struct Particle
    {
      alignas(16) Math::Vector3 position;
      uint32_t emitterIndex;
      float lifetime;
    };
  } // namespace Renderer
} // namespace Low
