#include "LowRenderer.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilFileIO.h"
#include "LowUtilString.h"
#include "LowUtilVariant.h"

#include "imgui.h"

#include "LowRendererBackend.h"
#include "LowRendererImage.h"
#include "LowRendererBuffer.h"
#include "LowRendererComputePipeline.h"
#include "LowRendererPipelineResourceSignature.h"
#include "LowRendererInterface.h"
#include "LowRendererComputeStepConfig.h"
#include "LowRendererGraphicsStepConfig.h"
#include "LowRendererGraphicsStep.h"
#include "LowRendererComputeStep.h"
#include "LowRendererRenderFlow.h"
#include "LowRendererTexture2D.h"
#include "LowRendererMesh.h"
#include "LowRendererMaterial.h"
#include "LowRendererMaterialType.h"
#include "LowRendererImGuiImage.h"
#include "LowRendererSkeleton.h"

#include "LowRendererResourceRegistry.h"
#include "LowRendererFrontendConfig.h"
#include "LowRendererCustomRenderSteps.h"

#include <stdint.h>

#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#include <gli/load_ktx.hpp>

#define LOW_RENDERER_MAX_POSE_BONES 512

namespace Low {
  namespace Renderer {
    struct PoseBone
    {
      uint32_t name;
      alignas(16) Math::Matrix4x4 transform;
    };

    struct PoseCalculation
    {
      Skeleton skeleton;
      SkeletalAnimation animation;
      float timestamp;
      uint32_t boneBufferStart;
    };

    struct MeshBufferFreeSlot
    {
      uint32_t start;
      uint32_t length;
    };

    namespace DynamicBufferType {
      enum Enum
      {
        VERTEX,
        INDEX,
        MISC
      };
    }

    struct DynamicBuffer
    {
      void initialize(Util::Name p_Name, Interface::Context p_Context,
                      uint8_t p_Type, uint32_t p_ElementSize,
                      uint32_t p_ElementCount)
      {
        LOW_ASSERT(!m_Initialized, "MeshBuffer already initialized");

        m_ElementSize = p_ElementSize;
        m_ElementCount = p_ElementCount;

        Backend::BufferCreateParams l_Params;
        l_Params.context = &p_Context.get_context();
        l_Params.bufferSize = p_ElementSize * p_ElementCount;
        l_Params.data = nullptr;
        if (p_Type == DynamicBufferType::VERTEX) {
          l_Params.usageFlags = LOW_RENDERER_BUFFER_USAGE_VERTEX |
                                LOW_RENDERER_BUFFER_USAGE_RESOURCE_BUFFER;
        } else if (p_Type == DynamicBufferType::INDEX) {
          l_Params.usageFlags = LOW_RENDERER_BUFFER_USAGE_INDEX;
        } else if (p_Type == DynamicBufferType::MISC) {
          l_Params.usageFlags = LOW_RENDERER_BUFFER_USAGE_RESOURCE_BUFFER;
        } else {
          LOW_ASSERT(false, "Unknown mesh buffer type");
        }
        m_Buffer = Resource::Buffer::make(p_Name, l_Params);
        m_FreeSlots.push_back({0, p_ElementCount});

        m_Type = p_Type;

        m_Initialized = true;
      }

      uint32_t reserve(uint32_t p_ElementCount)
      {
        LOW_ASSERT(m_Initialized, "Cannot write to uninitialized MeshBuffer");
        LOW_ASSERT(!m_FreeSlots.empty(), "No free space left in MeshBuffer");

        MeshBufferFreeSlot l_FreeSlot{0, 0};
        uint32_t i_SlotIndex = 0;

        for (; i_SlotIndex < m_FreeSlots.size(); ++i_SlotIndex) {
          if (m_FreeSlots[i_SlotIndex].length >= p_ElementCount) {
            l_FreeSlot = m_FreeSlots[i_SlotIndex];
            break;
          }
        }

        LOW_ASSERT(l_FreeSlot.length >= p_ElementCount,
                   "Could not find free space in MeshBuffer to fit data");

        uint32_t l_SavePoint = l_FreeSlot.start;
        if (l_FreeSlot.length == p_ElementCount) {
          m_FreeSlots.erase(m_FreeSlots.begin() + i_SlotIndex);
        } else {
          m_FreeSlots[i_SlotIndex].length = l_FreeSlot.length - p_ElementCount;
          m_FreeSlots[i_SlotIndex].start = l_SavePoint + p_ElementCount;
        }

        return l_SavePoint;
      }

      uint32_t write(void *p_DataPtr, uint32_t p_ElementCount)
      {
        uint32_t l_Offset = reserve(p_ElementCount);

        m_Buffer.write(p_DataPtr, p_ElementCount * m_ElementSize,
                       l_Offset * m_ElementSize);

        return l_Offset;
      }

      void free(uint32_t p_Position, uint32_t p_ElementCount)
      {
        LOW_ASSERT(m_Initialized, "Cannot free from uninitialized MeshBuffer");

        uint32_t l_ClosestUnder = ~0u;
        uint32_t l_ClosestOver = ~0u;
        uint32_t l_UnderDiff = ~0u;
        uint32_t l_OverDiff = ~0u;

        for (uint32_t i = 0u; i < m_FreeSlots.size(); ++i) {
          MeshBufferFreeSlot &i_Slot = m_FreeSlots[i];

          if (i_Slot.start < p_Position) {
            LOW_ASSERT((i_Slot.start + i_Slot.length) <= p_Position,
                       "Tried to double free from mesh buffer");

            uint32_t i_Diff = p_Position - (i_Slot.start + i_Slot.length);
            if (i_Diff < l_UnderDiff) {
              l_UnderDiff = i_Diff;
              l_ClosestUnder = i;
            }
          } else {
            LOW_ASSERT((p_Position + p_ElementCount) <= i_Slot.start,
                       "Tried to double free from mesh buffer");

            uint32_t i_Diff = i_Slot.start - (p_Position + p_ElementCount);
            if (i_Diff < l_OverDiff) {
              l_OverDiff = i_Diff;
              l_ClosestOver = i;
            }
          }
        }

        if (l_UnderDiff == 0 && l_OverDiff == 0) {
          m_FreeSlots[l_ClosestUnder].length =
              m_FreeSlots[l_ClosestUnder].length + p_ElementCount +
              m_FreeSlots[l_ClosestOver].length;

          m_FreeSlots.erase(m_FreeSlots.begin() + l_ClosestOver);
        } else if (l_UnderDiff == 0) {
          m_FreeSlots[l_ClosestUnder].length =
              m_FreeSlots[l_ClosestUnder].length + p_ElementCount;
        } else if (l_OverDiff == 0) {
          m_FreeSlots[l_ClosestOver].start = p_Position;
          m_FreeSlots[l_ClosestOver].length =
              m_FreeSlots[l_ClosestOver].length + p_ElementCount;
        } else {
          m_FreeSlots.push_back({p_Position, p_ElementCount});
        }
      }

      void clear()
      {
        m_FreeSlots.clear();
        m_FreeSlots.push_back({0, m_ElementCount});
      }

      void bind()
      {
        if (m_Type == DynamicBufferType::VERTEX) {
          m_Buffer.bind_vertex();
        } else if (m_Type == DynamicBufferType::INDEX) {
          m_Buffer.bind_index(Backend::IndexBufferType::UINT32);
        } else if (m_Type == DynamicBufferType::MISC) {
          LOW_ASSERT(false, "Cannot implicitly bind misc dynamic buffer");
        } else {
          LOW_ASSERT(false, "Unknown dynamic buffer type");
        }
      }

      Resource::Buffer m_Buffer;

    private:
      uint32_t m_ElementSize;
      uint32_t m_ElementCount;
      Util::List<MeshBufferFreeSlot> m_FreeSlots;
      bool m_Initialized = false;
      uint8_t m_Type;
    };

    struct RenderFlowUpdateData
    {
      RenderFlow renderflow;
      Math::UVector2 dimensions;
    };

    struct SkinningOperation
    {
      Mesh mesh;
      Skeleton skeleton;
      uint32_t skinningBufferStart;
      uint32_t poseBoneIndex;
      Math::Matrix4x4 transformation;
    };

    struct SkinningCalculationInput
    {
      uint32_t vertexStart;
      uint32_t vertexCount;
      uint32_t poseBoneStart;
      uint32_t postBoneCount;
      uint32_t skinningBufferStart;
      uint32_t vertexWeightBufferStart;
      uint32_t vertexWeightCount;
      uint32_t offset;
      Math::Matrix4x4 transformation;
    };

    Interface::Context g_Context;

    Interface::ComputePipeline g_SkinningPipeline;
    Interface::PipelineResourceSignature g_SkinningSignature;

    PoseBone g_PoseBones[LOW_RENDERER_MAX_POSE_BONES];
    uint32_t g_PoseBoneIndex = 0;

    Util::List<PoseCalculation> g_PendingPoseCalculations;
    Util::List<SkinningOperation> g_PendingSkinningOperations;

    Util::List<RenderFlowUpdateData> g_PendingRenderFlowUpdates;

    DynamicBuffer g_VertexBuffer;
    DynamicBuffer g_IndexBuffer;

    DynamicBuffer g_SkinningBuffer;

    DynamicBuffer g_VertexWeightBuffer;

    Resource::Buffer g_PoseBuffer;

    ResourceRegistry g_ResourceRegistry;
    Util::String g_ConfigPath;

    RenderFlow g_MainRenderFlow;
    Util::Name g_MainRenderFlowName;

    Interface::GraphicsPipeline g_FullscreenPipeline;
    Interface::PipelineResourceSignature g_FullScreenPipelineSignature;

    static void setup_custom_renderstep_configs()
    {
      ShadowStep::setup_config();
      SsaoStep::setup_config();
    }

    void adjust_renderflow_dimensions(RenderFlow p_RenderFlow,
                                      Math::UVector2 &p_Dimensions)
    {
      RenderFlowUpdateData l_UpdateData;
      l_UpdateData.renderflow = p_RenderFlow;
      l_UpdateData.dimensions = p_Dimensions;

      g_PendingRenderFlowUpdates.push_back(l_UpdateData);
    }

    Mesh upload_mesh(Util::Name p_Name, Util::Resource::MeshInfo &p_MeshInfo)
    {
      Mesh l_Mesh = Mesh::make(p_Name);

      l_Mesh.set_vertex_buffer_start(g_VertexBuffer.write(
          p_MeshInfo.vertices.data(), p_MeshInfo.vertices.size()));
      l_Mesh.set_index_buffer_start(g_IndexBuffer.write(
          p_MeshInfo.indices.data(), p_MeshInfo.indices.size()));
      l_Mesh.set_vertex_count(p_MeshInfo.vertices.size());
      l_Mesh.set_index_count(p_MeshInfo.indices.size());

      l_Mesh.set_vertexweight_buffer_start(0);
      l_Mesh.set_vertexweight_count(0);

      if (!p_MeshInfo.boneInfluences.empty()) {

        uint32_t l_WeightOffset = g_VertexWeightBuffer.write(
            p_MeshInfo.boneInfluences.data(), p_MeshInfo.boneInfluences.size());

        l_Mesh.set_vertexweight_buffer_start(l_WeightOffset);
        l_Mesh.set_vertexweight_count(p_MeshInfo.boneInfluences.size());
      }

      /*
      LOW_LOG_DEBUG << "UPLOADED: " << p_Name
                   << " VERTEX: " << l_Mesh.get_vertex_buffer_start() << " -> "
                   << l_Mesh.get_vertex_count() << " ("
                   << (l_Mesh.get_vertex_buffer_start() +
                       l_Mesh.get_vertex_count())
                   << ")"
                   << " INDEX: " << l_Mesh.get_index_buffer_start() << " -> "
                   << l_Mesh.get_index_count() << " ("
                   << (l_Mesh.get_index_buffer_start() +
                       l_Mesh.get_index_count())
                   << ")" << LOW_LOG_END;
      */

      return l_Mesh;
    }

    void unload_mesh(Mesh p_Mesh)
    {
      g_VertexBuffer.free(p_Mesh.get_vertex_buffer_start(),
                          p_Mesh.get_vertex_count());
      g_IndexBuffer.free(p_Mesh.get_index_buffer_start(),
                         p_Mesh.get_index_count());

      if (p_Mesh.get_vertexweight_count() > 0) {
        g_VertexWeightBuffer.free(p_Mesh.get_vertexweight_buffer_start(),
                                  p_Mesh.get_vertexweight_count());
      }

      p_Mesh.destroy();
    }

    static void parse_bone(Skeleton p_Skeleton, Util::Resource::Mesh &p_Mesh,
                           Util::Resource::Node &p_Node, Bone &p_Bone)
    {
      Util::Resource::Submesh &l_Submesh = p_Mesh.submeshes[p_Node.index];
      Util::Resource::Bone &l_Bone = p_Mesh.bones[l_Submesh.name];
      p_Bone.name = l_Submesh.name;
      p_Bone.index = l_Bone.index;
      p_Bone.offset = l_Bone.offset;
      p_Bone.parentTransformation = l_Submesh.parentTransform;
      p_Bone.localTransformation = l_Submesh.localTransform;
      p_Bone.children.resize(p_Node.children.size());

      for (uint32_t i = 0u; i < p_Node.children.size(); ++i) {
        parse_bone(p_Skeleton, p_Mesh, p_Node.children[i], p_Bone.children[i]);
      }
    }

    Skeleton upload_skeleton(Util::Name p_Name, Util::Resource::Mesh &p_Mesh)
    {
      Skeleton l_Skeleton = Skeleton::make(p_Name);

      parse_bone(l_Skeleton, p_Mesh, p_Mesh.rootNode,
                 l_Skeleton.get_root_bone());

      l_Skeleton.set_bone_count(p_Mesh.boneCount);

      l_Skeleton.get_animations().resize(p_Mesh.animations.size());
      for (uint32_t i = 0u; i < p_Mesh.animations.size(); ++i) {
        l_Skeleton.get_animations()[i] =
            SkeletalAnimation::make(p_Mesh.animations[i].name);

        l_Skeleton.get_animations()[i].set_duration(
            p_Mesh.animations[i].duration);

        l_Skeleton.get_animations()[i].set_ticks_per_second(
            p_Mesh.animations[i].ticksPerSecond);

        l_Skeleton.get_animations()[i].get_channels().resize(
            p_Mesh.animations[i].channels.size());

        for (uint32_t j = 0u; j < p_Mesh.animations[i].channels.size(); ++j) {
          l_Skeleton.get_animations()[i].get_channels()[j] =
              p_Mesh.animations[i].channels[j];
        }
      }

      return l_Skeleton;
    }

    void unload_skeleton(Skeleton p_Skeleton)
    {
      for (auto it = p_Skeleton.get_animations().begin();
           it != p_Skeleton.get_animations().end();) {
        SkeletalAnimation i_Animation = *it;
        it = p_Skeleton.get_animations().erase(it);
        i_Animation.destroy();
      }
      p_Skeleton.destroy();
    }

    Texture2D upload_texture(Util::Name p_Name,
                             Util::Resource::Image2D &p_Image)
    {
      return Texture2D::make(p_Name, g_Context, p_Image);
    }

    static void initialize_frontend_types()
    {
      GraphicsStepConfig::initialize();
      ComputeStepConfig::initialize();
      ComputeStep::initialize();
      GraphicsStep::initialize();
      RenderFlow::initialize();
      Texture2D::initialize();
      Mesh::initialize();
      MaterialType::initialize();
      Material::initialize();
      Skeleton::initialize();
    }

    static void initialize_resource_types()
    {
      Resource::Image::initialize();
      Resource::Buffer::initialize();
    }

    static void initialize_interface_types()
    {
      Interface::Context::initialize();
      Interface::Renderpass::initialize();
      Interface::PipelineResourceSignature::initialize();
      Interface::ComputePipeline::initialize();
      Interface::GraphicsPipeline::initialize();
      Interface::ImGuiImage::initialize();
    }

    static void initialize_types()
    {
      initialize_resource_types();
      initialize_interface_types();
      initialize_frontend_types();
    }

    static void initialize_global_resources()
    {
      Util::Yaml::Node l_RootNode = Util::Yaml::load_file(
          (g_ConfigPath + "/renderer_global_resources.yaml").c_str());

      /*
      g_ResourceRegistry.initialize(g_Context, l_RootNode);

      g_ResourceRegistry.get_buffer_resource(N(context_dimensions))
          .set(&g_Context.get_dimensions());
      */
    }

    static uint32_t
    material_type_place_properties_vector4(MaterialType p_MaterialType,
                                           uint32_t p_StartOffset)
    {
      uint32_t l_CurrentOffset = p_StartOffset;

      for (MaterialTypeProperty &i_Property : p_MaterialType.get_properties()) {
        if (i_Property.offset != ~0u) {
          // Skip properties that have already been placed
          continue;
        }
        if (i_Property.type != MaterialTypePropertyType::VECTOR4) {
          continue;
        }

        i_Property.offset = l_CurrentOffset;
        l_CurrentOffset += sizeof(Math::Vector4);
      }

      return l_CurrentOffset;
    }

    uint32_t
    material_type_place_properties_vector2(MaterialType p_MaterialType,
                                           uint32_t p_StartOffset,
                                           Util::List<uint32_t> &p_FreeSingles)
    {
      uint32_t l_CurrentOffset = p_StartOffset;
      uint32_t l_Vector2Count = 0;

      for (MaterialTypeProperty &i_Property : p_MaterialType.get_properties()) {
        if (i_Property.offset != ~0u) {
          // Skip properties that have already been placed
          continue;
        }
        if (i_Property.type != MaterialTypePropertyType::VECTOR2) {
          continue;
        }

        i_Property.offset = l_CurrentOffset;
        l_CurrentOffset += sizeof(Math::Vector2);
        l_Vector2Count++;
      }

      if (l_Vector2Count % 2 == 1) {
        // If there has been in odd number of vector2s that means that there are
        // two single fields free to be filled later on
        p_FreeSingles.push_back(l_CurrentOffset);
        p_FreeSingles.push_back(l_CurrentOffset + sizeof(uint32_t));

        l_CurrentOffset += sizeof(uint32_t) * 2;
      }

      return l_CurrentOffset;
    }

    uint32_t
    material_type_place_properties_vector3(MaterialType p_MaterialType,
                                           uint32_t p_StartOffset,
                                           Util::List<uint32_t> &p_FreeSingles)
    {
      uint32_t l_CurrentOffset = p_StartOffset;

      for (MaterialTypeProperty &i_Property : p_MaterialType.get_properties()) {
        if (i_Property.offset != ~0u) {
          // Skip properties that have already been placed
          continue;
        }
        if (i_Property.type != MaterialTypePropertyType::VECTOR3) {
          continue;
        }

        i_Property.offset = l_CurrentOffset;
        l_CurrentOffset += sizeof(Math::Vector3);
        p_FreeSingles.push_back(l_CurrentOffset);
        l_CurrentOffset += sizeof(uint32_t);
      }

      return l_CurrentOffset;
    }

    static void
    material_type_place_properties_single(MaterialType p_MaterialType,
                                          Util::List<uint32_t> &p_FreeSingles)
    {
      for (MaterialTypeProperty &i_Property : p_MaterialType.get_properties()) {
        if (i_Property.offset != ~0u) {
          // Skip properties that have already been placed
          continue;
        }

        LOW_ASSERT(!(i_Property.type == MaterialTypePropertyType::VECTOR3 ||
                     i_Property.type == MaterialTypePropertyType::VECTOR4 ||
                     i_Property.type == MaterialTypePropertyType::VECTOR2),
                   "Vector material property has not been placed yet");

        LOW_ASSERT(!p_FreeSingles.empty(), "No space left in material info");

        uint32_t i_Offset = p_FreeSingles.front();
        p_FreeSingles.erase_first(i_Offset);

        i_Property.offset = i_Offset;
      }
    }

    static void material_type_place_properties(MaterialType p_MaterialType)
    {
      uint32_t l_CurrentOffset = 0u;
      Util::List<uint32_t> l_FreeSingles;

      l_CurrentOffset = material_type_place_properties_vector4(p_MaterialType,
                                                               l_CurrentOffset);

      l_CurrentOffset = material_type_place_properties_vector2(
          p_MaterialType, l_CurrentOffset, l_FreeSingles);

      l_CurrentOffset = material_type_place_properties_vector3(
          p_MaterialType, l_CurrentOffset, l_FreeSingles);

      for (; l_CurrentOffset <
             LOW_RENDERER_MATERIAL_DATA_VECTORS * sizeof(Math::Vector4);
           l_CurrentOffset += sizeof(Math::Vector4)) {
        l_FreeSingles.push_back(l_CurrentOffset);
        l_FreeSingles.push_back(l_CurrentOffset + sizeof(float));
        l_FreeSingles.push_back(l_CurrentOffset + (sizeof(float) * 2));
        l_FreeSingles.push_back(l_CurrentOffset + (sizeof(float) * 3));
      }

      material_type_place_properties_single(p_MaterialType, l_FreeSingles);
    }

    static void load_material_types()
    {
      Util::String l_PipelineDirectory = g_ConfigPath + "/material_types";
      Util::List<Util::String> l_FilePaths;
      Util::FileIO::list_directory(l_PipelineDirectory.c_str(), l_FilePaths);
      Util::String l_Ending = ".materialtypes.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node = Util::Yaml::load_file(i_Path.c_str());

          for (auto it = i_Node.begin(); it != i_Node.end(); ++it) {
            Util::Name i_Name = LOW_YAML_AS_NAME(it->first);

            Util::Name i_GBufferPipelineName =
                LOW_YAML_AS_NAME(it->second["GBufferPipeline"]);
            Util::Name i_DepthPipelineName =
                LOW_YAML_AS_NAME(it->second["DepthPipeline"]);

            MaterialType i_MaterialType = MaterialType::make(i_Name);

            i_MaterialType.set_internal(false);
            if (it->second["internal"]) {
              i_MaterialType.set_internal(it->second["internal"].as<bool>());
            }

            i_MaterialType.set_gbuffer_pipeline(
                get_graphics_pipeline_config(i_GBufferPipelineName));
            i_MaterialType.set_depth_pipeline(
                get_graphics_pipeline_config(i_DepthPipelineName));

            uint32_t i_CurrentOffset = 0u;

            if (it->second["properties"]) {
              for (auto pit = it->second["properties"].begin();
                   pit != it->second["properties"].end(); ++pit) {
                Util::Name i_PropertyName = LOW_YAML_AS_NAME(pit->first);
                Util::String i_PropertyTypeString =
                    LOW_YAML_AS_STRING(pit->second["type"]);

                MaterialTypeProperty i_Property;
                i_Property.offset = ~0u;
                i_Property.name = i_PropertyName;

                if (i_PropertyTypeString == "Vector4") {
                  i_Property.type = MaterialTypePropertyType::VECTOR4;
                } else if (i_PropertyTypeString == "Texture2D") {
                  i_Property.type = MaterialTypePropertyType::TEXTURE2D;
                } else {
                  LOW_ASSERT(false, "Unknown materialtype property type");
                }

                i_MaterialType.get_properties().push_back(i_Property);
              }
            }

            material_type_place_properties(i_MaterialType);

            auto &props = i_MaterialType.get_properties();
          }
        }
      }
    }

    static void load_renderflows()
    {
      Util::String l_RenderFlowDirectory = g_ConfigPath + "/renderflows";
      Util::List<Util::String> l_FilePaths;
      Util::FileIO::list_directory(l_RenderFlowDirectory.c_str(), l_FilePaths);
      Util::String l_Ending = ".renderflow.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node = Util::Yaml::load_file(i_Path.c_str());
          Util::String i_Filename = i_Path.substr(
              l_RenderFlowDirectory.length() + 1, i_Path.length());
          Util::String i_Name =
              i_Filename.substr(0, i_Filename.length() - l_Ending.length());

          RenderFlow::make(LOW_NAME(i_Name.c_str()), g_Context, i_Node);

          LOW_LOG_DEBUG << "RenderFlow " << i_Name << " loaded" << LOW_LOG_END;
        }
      }
    }

    static void load_renderstep_configs()
    {
      Util::String l_RenderpassDirectory = g_ConfigPath + "/rendersteps";
      Util::List<Util::String> l_FilePaths;
      Util::FileIO::list_directory(l_RenderpassDirectory.c_str(), l_FilePaths);
      Util::String l_Ending = ".renderstep.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node = Util::Yaml::load_file(i_Path.c_str());
          Util::String i_Filename = i_Path.substr(
              l_RenderpassDirectory.length() + 1, i_Path.length());
          Util::String i_Name =
              i_Filename.substr(0, i_Filename.length() - l_Ending.length());

          if (LOW_YAML_AS_STRING(i_Node["type"]) == "compute") {
            ComputeStepConfig::make(LOW_NAME(i_Name.c_str()), i_Node);
          } else if (LOW_YAML_AS_STRING(i_Node["type"]) == "graphics") {
            GraphicsStepConfig::make(LOW_NAME(i_Name.c_str()), i_Node);
          } else {
            LOW_ASSERT(false, "Unknown renderstep type");
          }
          LOW_LOG_DEBUG << "Renderstep " << i_Name << " loaded" << LOW_LOG_END;
        }
      }
    }

    static void create_fullscreen_pipeline()
    {
      Util::List<Backend::GraphicsPipelineColorTarget> l_ColorTargets;
      {
        Backend::GraphicsPipelineColorTarget l_Target;
        l_Target.blendEnable = false;
        l_Target.wirteMask = LOW_RENDERER_COLOR_WRITE_BIT_RED |
                             LOW_RENDERER_COLOR_WRITE_BIT_GREEN |
                             LOW_RENDERER_COLOR_WRITE_BIT_BLUE |
                             LOW_RENDERER_COLOR_WRITE_BIT_ALPHA;
        l_ColorTargets.push_back(l_Target);
      }

      Interface::PipelineGraphicsCreateParams l_Params;
      l_Params.context = g_Context;
      l_Params.vertexShaderPath = "fs.vert";
      l_Params.fragmentShaderPath = "fs.frag";
      l_Params.dimensions = g_Context.get_dimensions();
      l_Params.signatures = {g_Context.get_global_signature(),
                             g_FullScreenPipelineSignature};
      l_Params.cullMode = Backend::PipelineRasterizerCullMode::BACK;
      l_Params.polygonMode = Backend::PipelineRasterizerPolygonMode::FILL;
      l_Params.frontFace = Backend::PipelineRasterizerFrontFace::CLOCKWISE;
      l_Params.dimensions = g_Context.get_dimensions();
      l_Params.renderpass = g_Context.get_renderpasses()[0];
      l_Params.colorTargets = l_ColorTargets;
      l_Params.vertexDataAttributeTypes = {};
      l_Params.depthWrite = false;
      l_Params.depthTest = false;
      l_Params.depthCompareOperation = Backend::CompareOperation::EQUAL;

      if (g_FullscreenPipeline.is_alive()) {
        Interface::PipelineManager::register_graphics_pipeline(
            g_FullscreenPipeline, l_Params);
      } else {
        g_FullscreenPipeline =
            Interface::GraphicsPipeline::make(N(FullscreenPipeline), l_Params);
      }
    }

    uint32_t calculate_skeleton_pose(Skeleton p_Skeleton,
                                     SkeletalAnimation p_Animation,
                                     float p_Timestamp)
    {
      uint32_t l_PoseIndex = g_PoseBoneIndex;
      g_PoseBoneIndex += p_Skeleton.get_bone_count();

      PoseCalculation l_Calculation;
      l_Calculation.boneBufferStart = l_PoseIndex;
      l_Calculation.animation = p_Animation;
      l_Calculation.skeleton = p_Skeleton;
      l_Calculation.timestamp = p_Timestamp;

      g_PendingPoseCalculations.push_back(l_Calculation);

      return l_PoseIndex;
    }

    uint32_t register_skinning_operation(Mesh p_Mesh, Skeleton p_Skeleton,
                                         uint32_t p_PoseIndex,
                                         Math::Matrix4x4 &p_Transformation)
    {
      SkinningOperation l_Operation;
      l_Operation.mesh = p_Mesh;
      l_Operation.skeleton = p_Skeleton;
      l_Operation.poseBoneIndex = p_PoseIndex;
      l_Operation.skinningBufferStart =
          g_SkinningBuffer.reserve(p_Mesh.get_vertex_count());

      LOW_ASSERT_WARN(p_Transformation == glm::mat4(1.0f), "REG");
      l_Operation.transformation = p_Transformation;

      g_PendingSkinningOperations.push_back(l_Operation);

      return l_Operation.skinningBufferStart;
    }

    void initialize()
    {
      g_ConfigPath = Util::String(LOW_DATA_PATH) + "/_internal/renderer_config";
      g_MainRenderFlowName = N(test);

      Backend::initialize();

      load_graphics_pipeline_configs(g_ConfigPath);

      initialize_types();

      load_renderstep_configs();

      Window l_Window;
      WindowInit l_WindowInit;
      l_WindowInit.dimensions.x = 1280;
      l_WindowInit.dimensions.y = 860;
      l_WindowInit.title = "LowEngine";

      window_initialize(l_Window, l_WindowInit);

#ifdef LOW_RENDERER_VALIDATION_ENABLED
      g_Context =
          Interface::Context::make(N(DefaultContext), &l_Window, 2, true);
#else
      g_Context =
          Interface::Context::make(N(DefaultContext), &l_Window, 2, false);
#endif

      g_VertexBuffer.initialize(N(VertexBuffer), g_Context,
                                DynamicBufferType::VERTEX,
                                sizeof(Util::Resource::Vertex), 128000u);
      g_IndexBuffer.initialize(N(IndexBuffer), g_Context,
                               DynamicBufferType::INDEX, sizeof(uint32_t),
                               256000u);

      g_SkinningBuffer.initialize(N(SkinningBuffer), g_Context,
                                  DynamicBufferType::VERTEX,
                                  sizeof(Util::Resource::Vertex), 20000u);

      g_VertexWeightBuffer.initialize(
          N(VertexWeightBuffer), g_Context, DynamicBufferType::MISC,
          sizeof(Util::Resource::BoneVertexWeight), 20000u);

      {
        Backend::BufferCreateParams l_Params;
        l_Params.context = &g_Context.get_context();
        l_Params.bufferSize = sizeof(PoseBone) * LOW_RENDERER_MAX_POSE_BONES;
        l_Params.data = nullptr;
        l_Params.usageFlags = LOW_RENDERER_BUFFER_USAGE_RESOURCE_BUFFER;
        g_PoseBuffer = Resource::Buffer::make(N(PoseBuffer), l_Params);
      }

      {
        Interface::PipelineComputeCreateParams l_Params;
        l_Params.context = g_Context;
        l_Params.shaderPath = "skinning.comp";

        Util::List<Backend::PipelineResourceDescription> l_ResourceDescriptions;

        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_VertexBuffer);
          l_Resource.step = Backend::ResourcePipelineStep::COMPUTE;
          l_Resource.type = Backend::ResourceType::BUFFER;
          l_Resource.arraySize = 1;

          l_ResourceDescriptions.push_back(l_Resource);
        }
        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_VertexWeightBuffer);
          l_Resource.step = Backend::ResourcePipelineStep::COMPUTE;
          l_Resource.type = Backend::ResourceType::BUFFER;
          l_Resource.arraySize = 1;

          l_ResourceDescriptions.push_back(l_Resource);
        }
        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_PoseBoneBuffer);
          l_Resource.step = Backend::ResourcePipelineStep::COMPUTE;
          l_Resource.type = Backend::ResourceType::BUFFER;
          l_Resource.arraySize = 1;

          l_ResourceDescriptions.push_back(l_Resource);
        }
        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_SkinningBuffer);
          l_Resource.step = Backend::ResourcePipelineStep::COMPUTE;
          l_Resource.type = Backend::ResourceType::BUFFER;
          l_Resource.arraySize = 1;

          l_ResourceDescriptions.push_back(l_Resource);
        }

        Interface::PipelineResourceSignature l_Signature =
            Interface::PipelineResourceSignature::make(
                N(SkinningSignature), g_Context, 1, l_ResourceDescriptions);

        l_Signature.set_buffer_resource(N(u_VertexBuffer), 0,
                                        g_VertexBuffer.m_Buffer);
        l_Signature.set_buffer_resource(N(u_VertexWeightBuffer), 0,
                                        g_VertexWeightBuffer.m_Buffer);
        l_Signature.set_buffer_resource(N(u_PoseBoneBuffer), 0, g_PoseBuffer);
        l_Signature.set_buffer_resource(N(u_SkinningBuffer), 0,
                                        g_SkinningBuffer.m_Buffer);

        g_SkinningSignature = l_Signature;

        l_Params.signatures = {g_Context.get_global_signature(), l_Signature};

        {
          Backend::PipelineConstantCreateParams l_Constant;
          l_Constant.name = N(inputInfo);
          // l_Constant.size = (sizeof(uint32_t) * 7) + (16 * 6);
          l_Constant.size = sizeof(SkinningCalculationInput);
          l_Params.constants.push_back(l_Constant);
        }

        g_SkinningPipeline =
            Interface::ComputePipeline::make(N(SkinningPipeline), l_Params);
      }

      initialize_global_resources();

      load_material_types();

      setup_custom_renderstep_configs();

      load_renderflows();

      {
        RenderFlow *l_RenderFlows = RenderFlow::living_instances();
        for (uint32_t i = 0; i < RenderFlow::living_count(); ++i) {
          if (l_RenderFlows[i].get_name() == g_MainRenderFlowName) {
            g_MainRenderFlow = l_RenderFlows[i];
            break;
          }
        }

        LOW_ASSERT(g_MainRenderFlow.is_alive(),
                   "Could not find main renderflow");
      }

      {
        Backend::PipelineResourceDescription l_ResourceDescription;
        l_ResourceDescription.name = N(u_FinalImage);
        l_ResourceDescription.arraySize = 1;
        l_ResourceDescription.step = Backend::ResourcePipelineStep::FRAGMENT;
        l_ResourceDescription.type = Backend::ResourceType::SAMPLER;
        Util::List<Backend::PipelineResourceDescription>
            l_ResourceDescriptions = {l_ResourceDescription};

        g_FullScreenPipelineSignature =
            Interface::PipelineResourceSignature::make(
                N(FullscreenPipelineSignature), g_Context, 1,
                l_ResourceDescriptions);

        create_fullscreen_pipeline();

        g_FullScreenPipelineSignature.set_sampler_resource(
            N(u_FinalImage), 0, Texture2D::ms_LivingInstances[0].get_image());
      }

      g_MainRenderFlow.set_camera_position(Math::Vector3(0.0f, 3.0f, 0.0f));
      g_MainRenderFlow.set_camera_direction(
          Math::VectorUtil::normalize(Math::Vector3(0.0f, 0.0f, -1.0f)));
    }

    void tick(float p_Delta)
    {
      g_Context.get_window().tick();

      Interface::PipelineManager::tick(p_Delta);

      uint8_t l_ContextState = g_Context.prepare_frame();

      LOW_ASSERT(l_ContextState != Backend::ContextState::FAILED,
                 "Frame prepare was not successful");

      g_Context.begin_imgui_frame();

      if (l_ContextState == Backend::ContextState::OUT_OF_DATE) {
        g_Context.update_dimensions();
        return;
      }

      g_PoseBoneIndex = 0;
      g_SkinningBuffer.clear();
      g_PendingPoseCalculations.clear();
      g_PendingSkinningOperations.clear();
    }

    static float calculate_interpolation_scale_factor(float t0, float t1,
                                                      float t)
    {
      float scaleFactor = 0.0f;
      float midWayLength = t - t0;
      float framesDiff = t1 - t0;
      scaleFactor = midWayLength / framesDiff;
      return scaleFactor;
    }

    static Math::Matrix4x4
    interpolate_bone_position(PoseCalculation &p_Calculation,
                              Util::Resource::AnimationChannel &p_Channel)
    {
      if (p_Channel.positions.size() == 1) {
        return glm::translate(glm::mat4(1.0), p_Channel.positions[0].value);
      }

      uint32_t l_Index0 = 0;
      uint32_t l_Index1 = 0;
      bool l_Found = false;
      for (; l_Index0 < p_Channel.positions.size() - 1; ++l_Index0) {
        if (p_Calculation.timestamp <
            p_Channel.positions[l_Index0 + 1].timestamp) {
          l_Index1 = l_Index0 + 1;
          l_Found = true;
          break;
        }
      }

      _LOW_ASSERT(l_Found);

      float l_ScaleFactor = calculate_interpolation_scale_factor(
          p_Channel.positions[l_Index0].timestamp,
          p_Channel.positions[l_Index1].timestamp, p_Calculation.timestamp);

      Math::Vector3 l_Position =
          glm::mix(p_Channel.positions[l_Index0].value,
                   p_Channel.positions[l_Index1].value, l_ScaleFactor);

      return glm::translate(glm::mat4(1.0f), l_Position);
    }

    static Math::Matrix4x4
    interpolate_bone_rotation(PoseCalculation &p_Calculation,
                              Util::Resource::AnimationChannel &p_Channel)
    {
      if (p_Channel.rotations.size() == 1) {
        glm::quat l_Quat0;
        l_Quat0.x = p_Channel.rotations[0].value.x;
        l_Quat0.y = p_Channel.rotations[0].value.y;
        l_Quat0.z = p_Channel.rotations[0].value.z;
        l_Quat0.w = p_Channel.rotations[0].value.w;
        l_Quat0 = glm::normalize(l_Quat0);
        return glm::toMat4(l_Quat0);
      }

      uint32_t l_Index0 = 0;
      uint32_t l_Index1 = 0;
      bool l_Found = false;
      for (; l_Index0 < p_Channel.rotations.size() - 1; ++l_Index0) {
        if (p_Calculation.timestamp <
            p_Channel.rotations[l_Index0 + 1].timestamp) {
          l_Index1 = l_Index0 + 1;
          l_Found = true;
          break;
        }
      }

      _LOW_ASSERT(l_Found);

      float l_ScaleFactor = calculate_interpolation_scale_factor(
          p_Channel.rotations[l_Index0].timestamp,
          p_Channel.rotations[l_Index1].timestamp, p_Calculation.timestamp);

      glm::quat l_Quat0;
      l_Quat0.x = p_Channel.rotations[l_Index0].value.x;
      l_Quat0.y = p_Channel.rotations[l_Index0].value.y;
      l_Quat0.z = p_Channel.rotations[l_Index0].value.z;
      l_Quat0.w = p_Channel.rotations[l_Index0].value.w;
      l_Quat0 = glm::normalize(l_Quat0);

      glm::quat l_Quat1;
      l_Quat1.x = p_Channel.rotations[l_Index1].value.x;
      l_Quat1.y = p_Channel.rotations[l_Index1].value.y;
      l_Quat1.z = p_Channel.rotations[l_Index1].value.z;
      l_Quat1.w = p_Channel.rotations[l_Index1].value.w;
      l_Quat1 = glm::normalize(l_Quat1);

      glm::quat l_Rotation =
          glm::normalize(glm::slerp(l_Quat0, l_Quat1, l_ScaleFactor));

      return glm::toMat4(l_Rotation);
    }

    static Math::Matrix4x4
    interpolate_bone_scale(PoseCalculation &p_Calculation,
                           Util::Resource::AnimationChannel &p_Channel)
    {
      if (p_Channel.scales.size() == 1) {
        return glm::scale(glm::mat4(1.0), p_Channel.scales[0].value);
      }

      uint32_t l_Index0 = 0;
      uint32_t l_Index1 = 0;
      bool l_Found = false;
      for (; l_Index0 < p_Channel.scales.size() - 1; ++l_Index0) {
        if (p_Calculation.timestamp <
            p_Channel.scales[l_Index0 + 1].timestamp) {
          l_Index1 = l_Index0 + 1;
          l_Found = true;
          break;
        }
      }

      _LOW_ASSERT(l_Found);

      float l_ScaleFactor = calculate_interpolation_scale_factor(
          p_Channel.scales[l_Index0].timestamp,
          p_Channel.scales[l_Index1].timestamp, p_Calculation.timestamp);

      Math::Vector3 l_Scale =
          glm::mix(p_Channel.scales[l_Index0].value,
                   p_Channel.scales[l_Index1].value, l_ScaleFactor);

      return glm::scale(glm::mat4(1.0f), l_Scale);
    }

    static void do_bone_calculation(PoseCalculation &p_Calculation,
                                    Bone &p_Bone, Math::Matrix4x4 &p_Transform)
    {
      bool l_FoundChannel = false;
      Math::Matrix4x4 l_GlobalTransform(1.0f);

      for (Util::Resource::AnimationChannel &i_Channel :
           p_Calculation.animation.get_channels()) {
        if (i_Channel.boneName == p_Bone.name) {
          g_PoseBones[p_Calculation.boneBufferStart + p_Bone.index].name =
              p_Bone.name.m_Index;

          Math::Matrix4x4 i_NodeTransform =
              interpolate_bone_position(p_Calculation, i_Channel) *
              interpolate_bone_rotation(p_Calculation, i_Channel) *
              interpolate_bone_scale(p_Calculation, i_Channel);

          l_GlobalTransform = p_Transform * i_NodeTransform;

          g_PoseBones[p_Calculation.boneBufferStart + p_Bone.index].transform =
              l_GlobalTransform * p_Bone.offset;

          l_FoundChannel = true;
          break;
        }
      }

      if (!l_FoundChannel) {
        g_PoseBones[p_Calculation.boneBufferStart + p_Bone.index].name =
            p_Bone.name.m_Index;
        l_GlobalTransform = p_Transform * p_Bone.localTransformation;
        g_PoseBones[p_Calculation.boneBufferStart + p_Bone.index].transform =
            l_GlobalTransform;
      }

      for (uint32_t i = 0u; i < p_Bone.children.size(); ++i) {
        do_bone_calculation(p_Calculation, p_Bone.children[i],
                            l_GlobalTransform);
      }
    }

    static void calculate_bone_matrices(PoseCalculation &p_Calculation)
    {
      Math::Matrix4x4 l_Transformation(1.0f);
      do_bone_calculation(p_Calculation, p_Calculation.skeleton.get_root_bone(),
                          l_Transformation);
    }

    static void do_skinning()
    {
      static Util::Name l_ConstantName = N(inputInfo);

      g_SkinningSignature.commit();

      g_SkinningPipeline.bind();
      for (uint32_t i = 0u; i < g_PendingSkinningOperations.size(); ++i) {
        SkinningOperation &i_Operation = g_PendingSkinningOperations[i];

        SkinningCalculationInput i_Input;
        i_Input.skinningBufferStart = i_Operation.skinningBufferStart;
        i_Input.poseBoneStart = i_Operation.poseBoneIndex;
        i_Input.postBoneCount = i_Operation.skeleton.get_bone_count();
        i_Input.vertexStart = i_Operation.mesh.get_vertex_buffer_start();
        i_Input.vertexCount = i_Operation.mesh.get_vertex_count();
        i_Input.vertexWeightBufferStart =
            i_Operation.mesh.get_vertexweight_buffer_start();
        i_Input.vertexWeightCount = i_Operation.mesh.get_vertexweight_count();
        i_Input.transformation = i_Operation.transformation;

        g_SkinningPipeline.set_constant(l_ConstantName, &i_Input);

        Backend::callbacks().compute_dispatch(
            g_Context.get_context(),
            {(i_Operation.mesh.get_vertex_count() / 256) + 1, 1, 1});
      }
    }

    void late_tick(float p_Delta)
    {
      if (g_Context.get_state() != Backend::ContextState::SUCCESS) {
        ImGui::EndFrame();
        ImGui::UpdatePlatformWindows();
        return;
      }

      static int x = 0;
      x += 1;

      g_VertexBuffer.bind();
      g_IndexBuffer.bind();

      g_Context.get_global_signature().commit();

      for (uint32_t i = 0u; i < g_PendingPoseCalculations.size(); ++i) {
        calculate_bone_matrices(g_PendingPoseCalculations[i]);
      }

      g_PoseBuffer.set(g_PoseBones);

      /*
      if (x == 100) {
        LOW_LOG_INFO << "Skinning" << LOW_LOG_END;
      }
      if (x >= 100) {
        do_skinning();
      }
      */
      do_skinning();

      g_MainRenderFlow.execute();

      if (g_Context.is_debug_enabled()) {
        LOW_RENDERER_BEGIN_RENDERDOC_SECTION(
            g_Context.get_context(), "FullscreenPass",
            Math::Color(0.3096f, 0.3461f, 0.3443f, 1.0f));
      }

      g_FullScreenPipelineSignature.commit();

      g_Context.get_current_renderpass().begin();

      g_FullscreenPipeline.bind();

      {
        Backend::DrawParams l_Params;
        l_Params.context = &g_Context.get_context();
        l_Params.firstVertex = 0;
        l_Params.vertexCount = 3;
        Backend::callbacks().draw(l_Params);
      }

      g_Context.render_imgui();

      g_Context.get_current_renderpass().end();

      if (g_Context.is_debug_enabled()) {
        LOW_RENDERER_END_RENDERDOC_SECTION(g_Context.get_context());
      }

      g_Context.render_frame();

      if (!g_PendingRenderFlowUpdates.empty()) {
        g_Context.wait_idle();
        for (RenderFlowUpdateData &i_Update : g_PendingRenderFlowUpdates) {
          i_Update.renderflow.update_dimensions(i_Update.dimensions);
        }
        g_PendingRenderFlowUpdates.clear();
        return;
      }
    }

    bool window_is_open()
    {
      return g_Context.get_window().is_open();
    }

    static void cleanup_frontend_types()
    {
      RenderFlow::cleanup();
      ComputeStep::cleanup();
      GraphicsStep::cleanup();
      ComputeStepConfig::cleanup();
      GraphicsStepConfig::cleanup();
      Material::cleanup();
      MaterialType::cleanup();
      Texture2D::cleanup();
      Mesh::cleanup();
      Skeleton::cleanup();
    }

    static void cleanup_resource_types()
    {
      Resource::Image::cleanup();
      Resource::Buffer::cleanup();
    }

    static void cleanup_interface_types()
    {
      Interface::ImGuiImage::cleanup();
      Interface::ComputePipeline::cleanup();
      Interface::GraphicsPipeline::cleanup();
      Interface::PipelineResourceSignature::cleanup();
      Interface::Renderpass::cleanup();
      Interface::Context::cleanup();
    }

    static void cleanup_types()
    {
      cleanup_frontend_types();
      cleanup_resource_types();
      cleanup_interface_types();
    }

    void cleanup()
    {
      g_Context.wait_idle();

      cleanup_types();
    }

    RenderFlow get_main_renderflow()
    {
      return g_MainRenderFlow;
    }

    Window &get_window()
    {
      return g_Context.get_window();
    }

    Material create_material(Util::Name, MaterialType p_Type)
    {
      Material l_Material =
          Renderer::Material::make(N(DebugGeometryMaterial), g_Context);
      l_Material.set_material_type(p_Type);

      return l_Material;
    }

    Resource::Buffer get_vertex_buffer()
    {
      return g_VertexBuffer.m_Buffer;
    }
    Resource::Buffer get_skinning_buffer()
    {
      return g_SkinningBuffer.m_Buffer;
    }
  } // namespace Renderer
} // namespace Low
