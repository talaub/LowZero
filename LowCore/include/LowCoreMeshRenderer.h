#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

#include "LowCoreEntity.h"

#include "LowRendererMesh.h"
#include "LowRendererMaterial.h"
#include "LowRendererRenderObject.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API MeshRenderer : public Low::Util::Handle
      {
      public:
        struct Data
        {
        public:
          Low::Renderer::Mesh mesh;
          Low::Renderer::Material material;
          Low::Renderer::RenderObject render_object;
          Low::Core::Entity entity;
          Low::Util::UniqueId unique_id;
          bool dirty;

          static size_t get_size()
          {
            return sizeof(Data);
          }
        };

      private:
        static u16 ms_TypeId;

      public:
        static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

        static Low::Util::List<MeshRenderer> ms_LivingInstances;

        const static Low::Util::TypeIdentifier IDENTIFIER;

        [[nodiscard]] static u16 type_id()
        {
          return ms_TypeId;
        }

        static MeshRenderer make(Low::Core::Entity p_Entity);
        static Low::Util::Handle _make(Low::Util::Handle p_Entity);
        static MeshRenderer make(Low::Core::Entity p_Entity,
                                 Low::Util::UniqueId p_UniqueId);
        explicit MeshRenderer(const MeshRenderer &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        MeshRenderer(u64 p_Id) : Low::Util::Handle(p_Id)
        {
        }
        MeshRenderer() : Low::Util::Handle()
        {
        }
        MeshRenderer(Low::Util::Handle p_Handle)
            : Low::Util::Handle(p_Handle.get_id())
        {
        }

        using Handle::operator=;

        MeshRenderer &operator=(const MeshRenderer &) = default;
        MeshRenderer &operator=(MeshRenderer &&) noexcept = default;

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static MeshRenderer *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static MeshRenderer create_handle_by_index(u32 p_Index);

        static MeshRenderer find_by_index(uint32_t p_Index);
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

        MeshRenderer duplicate(Low::Core::Entity p_Entity) const;
        static MeshRenderer duplicate(MeshRenderer p_Handle,
                                      Low::Core::Entity p_Entity);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Handle p_Entity);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Serial::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Serial::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          MeshRenderer l_Handle = p_Handle.get_id();
          return l_Handle.is_alive();
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          MeshRenderer l_MeshRenderer = p_Handle.get_id();
          l_MeshRenderer.destroy();
        }

        Low::Renderer::Mesh get_mesh() const;
        void set_mesh(Low::Renderer::Mesh p_Value);

        Low::Renderer::Material get_material() const;
        void set_material(Low::Renderer::Material p_Value);

        Low::Renderer::RenderObject get_render_object() const;
        void set_render_object(Low::Renderer::RenderObject p_Value);

        Low::Core::Entity get_entity() const;
        void set_entity(Low::Core::Entity p_Value);

        Low::Util::UniqueId get_unique_id() const;

        bool is_dirty() const;
        void set_dirty(bool p_Value);
        void toggle_dirty();
        void mark_dirty();

        static bool get_page_for_index(const u32 p_Index,
                                       u32 &p_PageIndex,
                                       u32 &p_SlotIndex);

      private:
        static u32 ms_Capacity;
        static u32 ms_PageSize;
        static u32 create_instance(u32 &p_PageIndex,
                                   u32 &p_SlotIndex);
        static u32 create_page();
        void set_unique_id(Low::Util::UniqueId p_Value);

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE

        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Component
  } // namespace Core
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
