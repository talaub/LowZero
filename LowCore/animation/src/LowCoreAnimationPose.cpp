#include "LowCoreAnimationPose.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowCoreAnimationBlender.h"
#include "LowCoreAnimationLocalPose.h"
#include "LowMathQuaternionUtil.h"
#include "LowMathVectorUtil.h"
#include "LowRendererAnimationClipState.h"
#include "LowRendererSkeletonState.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Animation {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      static Util::List<Math::Matrix4x4> g_GlobalPoseScratch;
      static LocalPose g_SamplePoseScratchA;
      static LocalPose g_SamplePoseScratchB;
      static LocalPose g_BlendPoseScratch;

      static float normalize_clip_time(float p_Time, float p_Duration,
                                       bool p_Looping)
      {
        if (p_Duration <= LOW_MATH_EPSILON) {
          return 0.0f;
        }

        if (p_Looping) {
          float l_Time = std::fmod(p_Time, p_Duration);
          if (l_Time < 0.0f) {
            l_Time += p_Duration;
          }
          return l_Time;
        }

        return Math::Util::clamp(p_Time, 0.0f, p_Duration);
      }

      static void
      extract_transform_components(const Math::Matrix4x4 &p_Transform,
                                   Math::Vector3 &p_Position,
                                   Math::Quaternion &p_Rotation,
                                   Math::Vector3 &p_Scale)
      {
        Math::Vector3 l_Skew;
        Math::Vector4 l_Perspective;
        glm::decompose(p_Transform, p_Scale, p_Rotation, p_Position,
                       l_Skew, l_Perspective);
        p_Rotation = Math::QuaternionUtil::normalize(p_Rotation);
      }

      static Math::Matrix4x4
      make_transform_matrix(const JointPose &p_Joint)
      {
        return glm::translate(Math::Matrix4x4(1.0f),
                              p_Joint.position) *
               glm::mat4_cast(p_Joint.rotation) *
               glm::scale(Math::Matrix4x4(1.0f), p_Joint.scale);
      }

      static Math::Matrix4x4
      make_transform_matrix(const Math::Vector3 &p_Position,
                            const Math::Quaternion &p_Rotation,
                            const Math::Vector3 &p_Scale)
      {
        return glm::translate(Math::Matrix4x4(1.0f), p_Position) *
               glm::mat4_cast(p_Rotation) *
               glm::scale(Math::Matrix4x4(1.0f), p_Scale);
      }

      static Math::Vector3 sample_vector_keys(
          const Util::List<Renderer::BinSerial::VecKey> &p_Keys,
          float p_Time, const Math::Vector3 &p_Default)
      {
        if (p_Keys.empty()) {
          return p_Default;
        }
        if (p_Keys.size() == 1u || p_Time <= p_Keys[0].time) {
          return p_Keys[0].value;
        }

        for (u32 i = 0u; i < p_Keys.size() - 1u; ++i) {
          const Renderer::BinSerial::VecKey &i_Current = p_Keys[i];
          const Renderer::BinSerial::VecKey &i_Next = p_Keys[i + 1u];

          if (p_Time <= i_Next.time) {
            const float i_Duration = i_Next.time - i_Current.time;
            if (i_Duration <= LOW_MATH_EPSILON) {
              return i_Current.value;
            }

            const float i_Factor =
                (p_Time - i_Current.time) / i_Duration;
            return Math::VectorUtil::lerp(i_Current.value,
                                          i_Next.value, i_Factor);
          }
        }

        return p_Keys[p_Keys.size() - 1u].value;
      }

      static Math::Quaternion sample_quat_keys(
          const Util::List<Renderer::BinSerial::QuatKey> &p_Keys,
          float p_Time, const Math::Quaternion &p_Default)
      {
        if (p_Keys.empty()) {
          return p_Default;
        }
        if (p_Keys.size() == 1u || p_Time <= p_Keys[0].time) {
          return Math::QuaternionUtil::normalize(p_Keys[0].value);
        }

        for (u32 i = 0u; i < p_Keys.size() - 1u; ++i) {
          const Renderer::BinSerial::QuatKey &i_Current = p_Keys[i];
          const Renderer::BinSerial::QuatKey &i_Next = p_Keys[i + 1u];

          if (p_Time <= i_Next.time) {
            const float i_Duration = i_Next.time - i_Current.time;
            if (i_Duration <= LOW_MATH_EPSILON) {
              return Math::QuaternionUtil::normalize(i_Current.value);
            }

            const float i_Factor =
                (p_Time - i_Current.time) / i_Duration;
            return Math::QuaternionUtil::normalize(
                Math::QuaternionUtil::slerp(i_Current.value,
                                            i_Next.value, i_Factor));
          }
        }

        return Math::QuaternionUtil::normalize(
            p_Keys[p_Keys.size() - 1u].value);
      }

      static bool sample_clip_to_local_pose(Clip p_Clip,
                                            float p_Progress,
                                            bool p_Looping,
                                            LocalPose &p_OutPose)
      {
        if (!p_Clip.is_alive() || !p_Clip.is_loaded()) {
          return false;
        }

        Renderer::AnimationClip l_Clip = p_Clip.get_renderer_clip();
        Renderer::Skeleton l_Skeleton = l_Clip.get_skeleton();
        if (!l_Skeleton.is_alive() ||
            l_Skeleton.get_state() !=
                Renderer::SkeletonState::LOADED) {
          return false;
        }

        Util::List<Renderer::SkeletonBone> &l_Bones =
            l_Skeleton.get_bones();
        const u32 l_BoneCount = static_cast<u32>(l_Bones.size());
        p_OutPose.resize(l_BoneCount);

        for (u32 i = 0u; i < l_BoneCount; ++i) {
          extract_transform_components(
              l_Bones[i].local_bind_transform,
              p_OutPose.joints[i].position,
              p_OutPose.joints[i].rotation,
              p_OutPose.joints[i].scale);
        }

        const float l_Time = normalize_clip_time(
            p_Progress, l_Clip.get_duration(), p_Looping);

        for (const Renderer::AnimationChannel &i_Channel :
             l_Clip.get_channels()) {
          if (i_Channel.bone_index >= l_BoneCount) {
            continue;
          }

          JointPose &i_Joint = p_OutPose.joints[i_Channel.bone_index];
          i_Joint.position = sample_vector_keys(
              i_Channel.positions, l_Time, i_Joint.position);
          i_Joint.rotation = sample_quat_keys(
              i_Channel.rotations, l_Time, i_Joint.rotation);
          i_Joint.scale = sample_vector_keys(i_Channel.scales, l_Time,
                                             i_Joint.scale);
        }

        return true;
      }

      static bool write_local_pose_to_skinning_pose(
          const LocalPose &p_LocalPose, Renderer::Skeleton p_Skeleton,
          Renderer::SkinningPose p_SkinningPose)
      {
        if (!p_Skeleton.is_alive() || !p_SkinningPose.is_alive()) {
          return false;
        }

        Util::List<Renderer::SkeletonBone> &l_Bones =
            p_Skeleton.get_bones();
        const u32 l_BoneCount = static_cast<u32>(l_Bones.size());
        if (p_LocalPose.joint_count() != l_BoneCount) {
          return false;
        }

        if (!p_SkinningPose.get_skeleton().is_alive() ||
            p_SkinningPose.get_skeleton().get_id() !=
                p_Skeleton.get_id()) {
          p_SkinningPose.set_skeleton(p_Skeleton);
        }

        Util::List<Math::Matrix4x4> &l_Matrices =
            p_SkinningPose.get_matrices();
        l_Matrices.resize(l_BoneCount);
        g_GlobalPoseScratch.resize(l_BoneCount);

        for (u32 i = 0u; i < l_BoneCount; ++i) {
          const Math::Matrix4x4 i_Local =
              make_transform_matrix(p_LocalPose.joints[i]);

          if (l_Bones[i].parent_index < 0) {
            g_GlobalPoseScratch[i] = i_Local;
          } else {
            g_GlobalPoseScratch[i] =
                g_GlobalPoseScratch[(u32)l_Bones[i].parent_index] *
                i_Local;
          }

          l_Matrices[i] =
              g_GlobalPoseScratch[i] * l_Bones[i].inverse_bind_matrix;
        }

        p_SkinningPose.mark_dirty();
        return true;
      }

      static bool sample_clip_to_skinning_pose(
          Clip p_Clip, float p_Progress, bool p_Looping,
          Renderer::SkinningPose p_SkinningPose)
      {
        if (!sample_clip_to_local_pose(p_Clip, p_Progress, p_Looping,
                                       g_SamplePoseScratchA)) {
          return false;
        }

        return write_local_pose_to_skinning_pose(
            g_SamplePoseScratchA,
            p_Clip.get_renderer_clip().get_skeleton(),
            p_SkinningPose);
      }

      static bool copy_skinning_pose(Renderer::SkinningPose p_Source,
                                     Renderer::SkinningPose p_Target)
      {
        if (!p_Source.is_alive() || !p_Target.is_alive()) {
          return false;
        }

        if (p_Source.get_skeleton().is_alive()) {
          p_Target.set_skeleton(p_Source.get_skeleton());
        }

        p_Target.get_matrices() = p_Source.get_matrices();
        p_Target.mark_dirty();
        return true;
      }

      static bool
      blend_skinning_pose_matrices(Renderer::SkinningPose p_SourceA,
                                   Renderer::SkinningPose p_SourceB,
                                   float p_Weight,
                                   Renderer::SkinningPose p_Target)
      {
        if (!p_SourceA.is_alive() || !p_SourceB.is_alive() ||
            !p_Target.is_alive()) {
          return false;
        }

        const Util::List<Math::Matrix4x4> l_SourceAMatrices =
            p_SourceA.get_matrices();
        const Util::List<Math::Matrix4x4> l_SourceBMatrices =
            p_SourceB.get_matrices();

        if (l_SourceAMatrices.size() != l_SourceBMatrices.size()) {
          return false;
        }
        if (p_SourceA.get_skeleton().is_alive() &&
            p_SourceB.get_skeleton().is_alive() &&
            p_SourceA.get_skeleton().get_id() !=
                p_SourceB.get_skeleton().get_id()) {
          return false;
        }

        if (p_SourceA.get_skeleton().is_alive()) {
          p_Target.set_skeleton(p_SourceA.get_skeleton());
        }

        const float l_Weight = Blender::normalize_weight(p_Weight);
        Util::List<Math::Matrix4x4> &l_TargetMatrices =
            p_Target.get_matrices();
        l_TargetMatrices.resize(l_SourceAMatrices.size());

        for (u32 i = 0u; i < l_SourceAMatrices.size(); ++i) {
          Math::Vector3 i_PosA(0.0f);
          Math::Quaternion i_RotA =
              Math::QuaternionUtil::get_identity();
          Math::Vector3 i_ScaleA(1.0f);
          Math::Vector3 i_PosB(0.0f);
          Math::Quaternion i_RotB =
              Math::QuaternionUtil::get_identity();
          Math::Vector3 i_ScaleB(1.0f);

          extract_transform_components(l_SourceAMatrices[i], i_PosA,
                                       i_RotA, i_ScaleA);
          extract_transform_components(l_SourceBMatrices[i], i_PosB,
                                       i_RotB, i_ScaleB);

          const Math::Vector3 i_Pos =
              Math::VectorUtil::lerp(i_PosA, i_PosB, l_Weight);
          const Math::Quaternion i_Rot =
              Math::QuaternionUtil::slerp(i_RotA, i_RotB, l_Weight);
          const Math::Vector3 i_Scale =
              Math::VectorUtil::lerp(i_ScaleA, i_ScaleB, l_Weight);

          l_TargetMatrices[i] =
              make_transform_matrix(i_Pos, i_Rot, i_Scale);
        }

        p_Target.mark_dirty();
        return true;
      }
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 Pose::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          Pose::IDENTIFIER(LOW_NAME(1181529166),
                           LOW_NAME(2416501569));
      uint32_t Pose::ms_Capacity = 0u;
      uint32_t Pose::ms_PageSize = 0u;
      Low::Util::List<Pose> Pose::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *> Pose::ms_Pages;

      Low::Util::Handle Pose::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Pose Pose::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        Pose l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Pose::ms_TypeId;

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Pose, skinning_pose,
                                   Renderer::SkinningPose))
            Renderer::SkinningPose();
        ACCESSOR_TYPE_SOA(l_Handle, Pose, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        l_Handle.set_skinning_pose(
            Renderer::SkinningPose::make(p_Name));
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Pose::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          if (get_skinning_pose().is_alive()) {
            get_skinning_pose().destroy();
          }
          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        _LOW_ASSERT(get_page_for_index(get_index(), l_PageIndex,
                                       l_SlotIndex));
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

        l_Page->slots[l_SlotIndex].m_Occupied = false;
        l_Page->slots[l_SlotIndex].m_Generation++;

        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end();) {
          if (it->get_id() == get_id()) {
            it = ms_LivingInstances.erase(it);
          } else {
            it++;
          }
        }
      }

      void Pose::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(N(LowCore),
                                                          N(Pose));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(Pose));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Pose::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Pose);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Pose::is_alive;
        l_TypeInfo.destroy = &Pose::destroy;
        l_TypeInfo.serialize = &Pose::serialize;
        l_TypeInfo.deserialize = &Pose::deserialize;
        l_TypeInfo.find_by_index = &Pose::_find_by_index;
        l_TypeInfo.notify = &Pose::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &Pose::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Pose::_make;
        l_TypeInfo.duplicate_default = &Pose::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Pose::living_instances);
        l_TypeInfo.get_living_count = &Pose::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: skinning_pose
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(skinning_pose);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Pose::Data, skinning_pose);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Renderer::SkinningPose::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Pose l_Handle = p_Handle.get_id();
            l_Handle.get_skinning_pose();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Pose,
                                              skinning_pose,
                                              Renderer::SkinningPose);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Pose l_Handle = p_Handle.get_id();
            l_Handle.set_skinning_pose(
                *(Renderer::SkinningPose *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Pose l_Handle = p_Handle.get_id();
            *((Renderer::SkinningPose *)p_Data) =
                l_Handle.get_skinning_pose();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: skinning_pose
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Pose::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Pose l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Pose, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Pose l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Pose l_Handle = p_Handle.get_id();
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Function: copy_from
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(copy_from);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Source);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Pose::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: copy_from
        }
        {
          // Function: blend_with
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(blend_with);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Target);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Pose::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Weight);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: blend_with
        }
        {
          // Function: blend_from
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(blend_from);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_SourceA);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Pose::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_SourceB);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Pose::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Weight);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: blend_from
        }
        {
          // Function: sample_clip
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(sample_clip);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Clip);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Clip::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Progress);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Looping);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::BOOL;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: sample_clip
        }
        {
          // Function: blend_from_clips
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(blend_from_clips);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_SourceA);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Clip::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_ProgressA);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_SourceB);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Clip::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_ProgressB);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Weight);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Looping);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::BOOL;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: blend_from_clips
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void Pose::cleanup()
      {
        Low::Util::List<Pose> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
          Low::Util::Instances::Page *i_Page = *it;
          free(i_Page->buffer);
          free(i_Page->slots);
          delete i_Page;
          it = ms_Pages.erase(it);
        }

        ms_Capacity = 0;
      }

      Low::Util::Handle Pose::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Pose Pose::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Pose l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Pose::ms_TypeId;

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
          l_Handle.m_Data.m_Generation = 0;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        l_Handle.m_Data.m_Generation =
            l_Page->slots[l_SlotIndex].m_Generation;

        return l_Handle;
      }

      Pose Pose::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Pose l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Pose::ms_TypeId;

        return l_Handle;
      }

      bool Pose::is_alive() const
      {
        if (m_Data.m_Type != Pose::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == Pose::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Pose::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle Pose::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      Pose Pose::find_by_name(Low::Util::Name p_Name)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:FIND_BY_NAME
        // LOW_CODEGEN::END::CUSTOM:FIND_BY_NAME

        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
        return Low::Util::Handle::DEAD;
      }

      Pose Pose::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        Pose l_Handle = make(p_Name);
        if (get_skinning_pose().is_alive()) {
          l_Handle.set_skinning_pose(get_skinning_pose());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      Pose Pose::duplicate(Pose p_Handle, Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle Pose::_duplicate(Low::Util::Handle p_Handle,
                                         Low::Util::Name p_Name)
      {
        Pose l_Pose = p_Handle.get_id();
        return l_Pose.duplicate(p_Name);
      }

      void Pose::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        if (get_skinning_pose().is_alive()) {
          get_skinning_pose().serialize(p_Node["skinning_pose"]);
        }
        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Pose::serialize(Low::Util::Handle p_Handle,
                           Low::Util::Serial::Node &p_Node)
      {
        Pose l_Pose = p_Handle.get_id();
        l_Pose.serialize(p_Node);
      }

      Low::Util::Handle
      Pose::deserialize(Low::Util::Serial::Node &p_Node,
                        Low::Util::Handle p_Creator)
      {
        Pose l_Handle = Pose::make(N(Pose));

        if (p_Node["skinning_pose"]) {
          l_Handle.set_skinning_pose(
              Renderer::SkinningPose::deserialize(
                  p_Node["skinning_pose"], l_Handle.get_id())
                  .get_id());
        }
        if (p_Node["name"]) {
          l_Handle.set_name(p_Node["name"].as<Low::Util::Name>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void
      Pose::broadcast_observable(Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 Pose::observe(Low::Util::Name p_Observable,
                        Low::Util::Function<void(Low::Util::Handle,
                                                 Low::Util::Name)>
                            p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Pose::observe(Low::Util::Name p_Observable,
                        Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Pose::notify(Low::Util::Handle p_Observed,
                        Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Pose::_notify(Low::Util::Handle p_Observer,
                         Low::Util::Handle p_Observed,
                         Low::Util::Name p_Observable)
      {
        Pose l_Pose = p_Observer.get_id();
        l_Pose.notify(p_Observed, p_Observable);
      }

      Renderer::SkinningPose Pose::get_skinning_pose() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_skinning_pose
        // LOW_CODEGEN::END::CUSTOM:GETTER_skinning_pose

        return TYPE_SOA(Pose, skinning_pose, Renderer::SkinningPose);
      }
      void Pose::set_skinning_pose(Renderer::SkinningPose p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_skinning_pose
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_skinning_pose

        // Set new value
        TYPE_SOA(Pose, skinning_pose, Renderer::SkinningPose) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_skinning_pose
        // LOW_CODEGEN::END::CUSTOM:SETTER_skinning_pose

        broadcast_observable(N(skinning_pose));
      }

      Low::Util::Name Pose::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Pose, name, Low::Util::Name);
      }
      void Pose::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(Pose, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      bool Pose::copy_from(Pose p_Source)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_copy_from
        if (!is_alive() || !p_Source.is_alive()) {
          return false;
        }

        return copy_skinning_pose(p_Source.get_skinning_pose(),
                                  get_skinning_pose());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_copy_from
      }

      bool Pose::blend_with(Pose p_Target, float p_Weight)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_blend_with
        if (!is_alive() || !p_Target.is_alive()) {
          return false;
        }

        return blend_skinning_pose_matrices(
            get_skinning_pose(), p_Target.get_skinning_pose(),
            p_Weight, get_skinning_pose());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_blend_with
      }

      bool Pose::blend_from(Pose p_SourceA, Pose p_SourceB,
                            float p_Weight)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_blend_from
        if (!is_alive() || !p_SourceA.is_alive() ||
            !p_SourceB.is_alive()) {
          return false;
        }

        return blend_skinning_pose_matrices(
            p_SourceA.get_skinning_pose(),
            p_SourceB.get_skinning_pose(), p_Weight,
            get_skinning_pose());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_blend_from
      }

      bool Pose::sample_clip(Clip p_Clip, float p_Progress,
                             bool p_Looping)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_sample_clip
        if (!is_alive()) {
          return false;
        }

        return sample_clip_to_skinning_pose(
            p_Clip, p_Progress, p_Looping, get_skinning_pose());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_sample_clip
      }

      bool Pose::blend_from_clips(Clip p_SourceA, float p_ProgressA,
                                  Clip p_SourceB, float p_ProgressB,
                                  float p_Weight, bool p_Looping)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_blend_from_clips
        if (!is_alive()) {
          return false;
        }
        if (!p_SourceA.is_alive() || !p_SourceB.is_alive()) {
          return false;
        }

        if (!sample_clip_to_local_pose(p_SourceA, p_ProgressA,
                                       p_Looping,
                                       g_SamplePoseScratchA)) {
          return false;
        }
        if (!sample_clip_to_local_pose(p_SourceB, p_ProgressB,
                                       p_Looping,
                                       g_SamplePoseScratchB)) {
          return false;
        }

        Renderer::Skeleton l_SourceASkeleton =
            p_SourceA.get_renderer_clip().get_skeleton();
        Renderer::Skeleton l_SourceBSkeleton =
            p_SourceB.get_renderer_clip().get_skeleton();
        if (!l_SourceASkeleton.is_alive() ||
            !l_SourceBSkeleton.is_alive() ||
            l_SourceASkeleton.get_id() !=
                l_SourceBSkeleton.get_id()) {
          return false;
        }

        if (!Blender::blend(g_SamplePoseScratchA,
                            g_SamplePoseScratchB, p_Weight,
                            g_BlendPoseScratch)) {
          return false;
        }

        return write_local_pose_to_skinning_pose(g_BlendPoseScratch,
                                                 l_SourceASkeleton,
                                                 get_skinning_pose());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_blend_from_clips
      }

      uint32_t Pose::create_instance(u32 &p_PageIndex,
                                     u32 &p_SlotIndex)
      {
        u32 l_Index = 0;
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        bool l_FoundIndex = false;

        for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
             ++l_PageIndex) {
          for (l_SlotIndex = 0;
               l_SlotIndex < ms_Pages[l_PageIndex]->size;
               ++l_SlotIndex) {
            if (!ms_Pages[l_PageIndex]
                     ->slots[l_SlotIndex]
                     .m_Occupied) {
              l_FoundIndex = true;
              break;
            }
            l_Index++;
          }
          if (l_FoundIndex) {
            break;
          }
        }
        if (!l_FoundIndex) {
          l_SlotIndex = 0;
          l_PageIndex = create_page();
        }
        ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
        p_PageIndex = l_PageIndex;
        p_SlotIndex = l_SlotIndex;
        return l_Index;
      }

      u32 Pose::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Pose.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Pose::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Pose::get_page_for_index(const u32 p_Index,
                                    u32 &p_PageIndex,
                                    u32 &p_SlotIndex)
      {
        if (p_Index >= get_capacity()) {
          p_PageIndex = LOW_UINT32_MAX;
          p_SlotIndex = LOW_UINT32_MAX;
          return false;
        }
        p_PageIndex = p_Index / ms_PageSize;
        if (p_PageIndex > (ms_Pages.size() - 1)) {
          return false;
        }
        p_SlotIndex = p_Index - (ms_PageSize * p_PageIndex);
        return true;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Animation
  } // namespace Core
} // namespace Low
