#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererGpuSubmesh.h"
#include "LowRendererSkinningInstance.h"
#include "LowRendererGlobals.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API SkinningCommand
        : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        SkinningInstance instance;
        GpuSubmesh submesh;
        uint32_t weights_start;
        uint32_t skinned_vertex_start;
        uint32_t vertex_count;
        VertexBuffer active_vertex_buffer;
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

      static Low::Util::List<SkinningCommand> ms_LivingInstances;

      const static Low::Util::TypeIdentifier IDENTIFIER;

      [[nodiscard]] static u16 type_id()
      {
        return ms_TypeId;
      }

    private:
      static SkinningCommand make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit SkinningCommand(const SkinningCommand &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      SkinningCommand(u64 p_Id) : Low::Util::Handle(p_Id)
      {
      }
      SkinningCommand() : Low::Util::Handle()
      {
      }
      SkinningCommand(Low::Util::Handle p_Handle)
          : Low::Util::Handle(p_Handle.get_id())
      {
      }

      using Handle::operator=;

      SkinningCommand &operator=(const SkinningCommand &) = default;
      SkinningCommand &
      operator=(SkinningCommand &&) noexcept = default;

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static SkinningCommand *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static SkinningCommand create_handle_by_index(u32 p_Index);

      static SkinningCommand find_by_index(uint32_t p_Index);
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

      SkinningCommand duplicate(Low::Util::Name p_Name) const;
      static SkinningCommand duplicate(SkinningCommand p_Handle,
                                       Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static SkinningCommand find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Serial::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Serial::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        SkinningCommand l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        SkinningCommand l_SkinningCommand = p_Handle.get_id();
        l_SkinningCommand.destroy();
      }

      SkinningInstance get_instance() const;

      GpuSubmesh get_submesh() const;

      uint32_t get_weights_start() const;
      void set_weights_start(uint32_t p_Value);

      uint32_t get_skinned_vertex_start() const;
      void set_skinned_vertex_start(uint32_t p_Value);

      uint32_t get_vertex_count() const;

      VertexBuffer get_active_vertex_buffer() const;
      void set_active_vertex_buffer(VertexBuffer p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static SkinningCommand make(SkinningInstance p_Instance,
                                  GpuSubmesh p_Submesh);
      static bool get_page_for_index(const u32 p_Index,
                                     u32 &p_PageIndex,
                                     u32 &p_SlotIndex);

    private:
      static u32 ms_Capacity;
      static u32 ms_PageSize;
      static u32 create_instance(u32 &p_PageIndex, u32 &p_SlotIndex);
      static u32 create_page();
      void set_instance(SkinningInstance p_Value);
      void set_submesh(GpuSubmesh p_Value);
      void set_vertex_count(uint32_t p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
