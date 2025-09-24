#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererGpuSubmesh.h"
#include "LowRendererMaterial.h"
#include "LowRendererUiRenderObject.h"
#include "LowRendererTexture.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    struct UiCanvas;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API UiDrawCommand : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        Low::Renderer::UiRenderObject render_object;
        uint64_t canvas_handle;
        Texture texture;
        Low::Math::Vector3 position;
        Low::Math::Vector2 size;
        float rotation2D;
        Low::Math::Color color;
        Low::Math::Vector4 uv_rect;
        Material material;
        uint32_t z_sorting;
        Low::Renderer::GpuSubmesh submesh;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(Data);
        }
      };

    public:
      static Low::Util::SharedMutex ms_LivingMutex;
      static Low::Util::UniqueLock<Low::Util::SharedMutex>
          ms_PagesLock;
      static Low::Util::SharedMutex ms_PagesMutex;
      static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

      static Low::Util::List<UiDrawCommand> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      UiDrawCommand();
      UiDrawCommand(uint64_t p_Id);
      UiDrawCommand(UiDrawCommand &p_Copy);

    private:
      static UiDrawCommand make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit UiDrawCommand(const UiDrawCommand &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      static uint32_t living_count()
      {
        Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static UiDrawCommand *living_instances()
      {
        Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        return ms_LivingInstances.data();
      }

      static UiDrawCommand create_handle_by_index(u32 p_Index);

      static UiDrawCommand find_by_index(uint32_t p_Index);
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

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      UiDrawCommand duplicate(Low::Util::Name p_Name) const;
      static UiDrawCommand duplicate(UiDrawCommand p_Handle,
                                     Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static UiDrawCommand find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        UiDrawCommand l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        UiDrawCommand l_UiDrawCommand = p_Handle.get_id();
        l_UiDrawCommand.destroy();
      }

      Low::Renderer::UiRenderObject get_render_object() const;
      void set_render_object(Low::Renderer::UiRenderObject p_Value);

      uint64_t get_canvas_handle() const;
      void set_canvas_handle(uint64_t p_Value);

      Texture get_texture() const;
      void set_texture(Texture p_Value);

      Low::Math::Vector3 &get_position() const;
      void set_position(Low::Math::Vector3 &p_Value);
      void set_position(float p_X, float p_Y, float p_Z);
      void set_position_x(float p_Value);
      void set_position_y(float p_Value);
      void set_position_z(float p_Value);

      Low::Math::Vector2 &get_size() const;
      void set_size(Low::Math::Vector2 &p_Value);
      void set_size(float p_X, float p_Y);
      void set_size_x(float p_Value);
      void set_size_y(float p_Value);

      float get_rotation2D() const;
      void set_rotation2D(float p_Value);

      Low::Math::Color &get_color() const;
      void set_color(Low::Math::Color &p_Value);

      Low::Math::Vector4 &get_uv_rect() const;
      void set_uv_rect(Low::Math::Vector4 &p_Value);

      Material get_material() const;
      void set_material(Material p_Value);

      uint32_t get_z_sorting() const;
      void set_z_sorting(uint32_t p_Value);

      Low::Renderer::GpuSubmesh get_submesh() const;

      void mark_dirty();

      void mark_z_dirty();

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static UiDrawCommand
      make(Low::Renderer::UiRenderObject p_RenderObject,
           Low::Renderer::UiCanvas p_Canvas,
           Low::Renderer::GpuSubmesh p_Submesh);
      static bool get_page_for_index(const u32 p_Index,
                                     u32 &p_PageIndex,
                                     u32 &p_SlotIndex);

    private:
      static u32 ms_Capacity;
      static u32 ms_PageSize;
      static u32 create_instance(
          u32 &p_PageIndex, u32 &p_SlotIndex,
          Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock);
      static u32 create_page();
      void set_submesh(Low::Renderer::GpuSubmesh p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
