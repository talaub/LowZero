#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererDrawCommand.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API MeshInstanceNode
        : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        Low::Math::Matrix4x4 world_transform;
        int32_t parent_index;
        int32_t bone_index;
        DrawCommand draw_command;
        bool dirty;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(Data);
        }
      };

    private:
      static u16 ms_TypeId;

    public:
      static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

      static Low::Util::List<MeshInstanceNode> ms_LivingInstances;

      const static Low::Util::TypeIdentifier IDENTIFIER;

      [[nodiscard]] static u16 type_id()
      {
        return ms_TypeId;
      }

    private:
      static MeshInstanceNode make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit MeshInstanceNode(const MeshInstanceNode &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      MeshInstanceNode(u64 p_Id) : Low::Util::Handle(p_Id)
      {
      }
      MeshInstanceNode() : Low::Util::Handle()
      {
      }
      MeshInstanceNode(Low::Util::Handle p_Handle)
          : Low::Util::Handle(p_Handle.get_id())
      {
      }

      using Handle::operator=;

      MeshInstanceNode &operator=(const MeshInstanceNode &) = default;
      MeshInstanceNode &
      operator=(MeshInstanceNode &&) noexcept = default;

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static MeshInstanceNode *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static MeshInstanceNode create_handle_by_index(u32 p_Index);

      static MeshInstanceNode find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      u64 observe(Low::Util::Name p_Observable,
                  Low::Util::Handle p_Observer) const;
      u64 observe(Low::Util::Name p_Observable,
                  Low::Util::Function<void(Low::Util::Handle,
                                           Low::Util::Name)>
                      p_Observer) const;
      void notify(Low::Util::Handle p_Observed,
                  Low::Util::Name p_Observable);
      void broadcast_observable(Low::Util::Name p_Observable) const;

      static void _notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable);

      static uint32_t get_capacity();

      void serialize(Low::Util::Serial::Node &p_Node) const;

      MeshInstanceNode duplicate(Low::Util::Name p_Name) const;
      static MeshInstanceNode duplicate(MeshInstanceNode p_Handle,
                                        Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static MeshInstanceNode find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Serial::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Serial::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        MeshInstanceNode l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        MeshInstanceNode l_MeshInstanceNode = p_Handle.get_id();
        l_MeshInstanceNode.destroy();
      }

      Low::Math::Matrix4x4 &get_world_transform() const;
      void set_world_transform(Low::Math::Matrix4x4 &p_Value);

      int32_t get_parent_index() const;

      int32_t get_bone_index() const;

      DrawCommand get_draw_command() const;
      void set_draw_command(DrawCommand p_Value);

      bool is_dirty() const;
      void set_dirty(bool p_Value);
      void toggle_dirty();
      void mark_dirty();

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static MeshInstanceNode make(Util::Name p_Name,
                                   int32_t p_ParentIndex,
                                   int32_t p_BoneIndex);
      static bool get_page_for_index(const u32 p_Index,
                                     u32 &p_PageIndex,
                                     u32 &p_SlotIndex);

    private:
      static u32 ms_Capacity;
      static u32 ms_PageSize;
      static u32 create_instance(u32 &p_PageIndex, u32 &p_SlotIndex);
      static u32 create_page();
      void set_parent_index(int32_t p_Value);
      void set_bone_index(int32_t p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
