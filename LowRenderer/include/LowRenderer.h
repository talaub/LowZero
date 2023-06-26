#pragma once

#include "LowRendererApi.h"
#include "LowRendererExposedObjects.h"
#include "LowRendererRenderFlow.h"
#include "LowRendererWindow.h"

#include "LowUtilEnums.h"
#include "LowUtilResource.h"

#include "LowMath.h"

namespace Low {
  namespace Renderer {
    struct RenderFlow;

    LOW_RENDERER_API void initialize();
    LOW_RENDERER_API void tick(float p_Delta, Util::EngineState p_State);
    LOW_RENDERER_API void late_tick(float p_Delta, Util::EngineState p_State);
    LOW_RENDERER_API bool window_is_open();
    LOW_RENDERER_API Window &get_window();
    LOW_RENDERER_API void cleanup();
    LOW_RENDERER_API void
    adjust_renderflow_dimensions(RenderFlow p_RenderFlow,
                                 Math::UVector2 &p_Dimensions);

    LOW_RENDERER_API Mesh upload_mesh(Util::Name p_Name,
                                      Util::Resource::MeshInfo &p_MeshInfo);
    LOW_RENDERER_API void unload_mesh(Mesh p_Mesh);

    LOW_RENDERER_API RenderFlow get_main_renderflow();

    LOW_RENDERER_API Material create_material(Util::Name p_Name,
                                              MaterialType p_Type);

    LOW_RENDERER_API Texture2D upload_texture(Util::Name p_Name,
                                              Util::Resource::Image2D &p_Image);

    LOW_RENDERER_API Skeleton upload_skeleton(Util::Name p_Name,
                                              Util::Resource::Mesh &p_Mesh);
    LOW_RENDERER_API void unload_skeleton(Skeleton p_Skeleton);

    LOW_RENDERER_API uint32_t register_skinning_operation(
        Mesh p_Mesh, Skeleton p_Skeleton, uint32_t p_PoseIndex,
        Math::Matrix4x4 &p_Transformation);

    LOW_RENDERER_API uint32_t calculate_skeleton_pose(
        Skeleton p_Skeleton, SkeletalAnimation p_Animation, float p_Timestamp);

    Resource::Buffer get_vertex_buffer();
    Resource::Buffer get_skinning_buffer();
    Resource::Buffer get_particle_emitter_buffer();
    Resource::Buffer get_particle_buffer();

  } // namespace Renderer
} // namespace Low
