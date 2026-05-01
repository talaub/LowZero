#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

#include "LowRendererSS2DDrawCommand.h"
#include "LowRendererTexture.h"
#include "LowRendererBuffer.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API SS2DCanvas : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        Low::Util::List<SS2DDrawCommand> draw_commands;
        Low::Renderer::Texture out_image;
        Low::Math::UVector2 dimensions;
        Low::Math::UVector2 desired_dimensions;
        Low::Renderer::Buffer drawcommand_index_buffer;
        uint64_t backend_handle;
        bool z_dirty;
        bool dimensions_dirty;
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

      static Low::Util::List<SS2DCanvas> ms_LivingInstances;

      const static Low::Util::TypeIdentifier IDENTIFIER;

      [[nodiscard]] static u16 type_id()
      {
        return ms_TypeId;
      }

      static SS2DCanvas make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit SS2DCanvas(const SS2DCanvas &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      SS2DCanvas(u64 p_Id) : Low::Util::Handle(p_Id)
      {
      }
      SS2DCanvas() : Low::Util::Handle()
      {
      }
      SS2DCanvas(Low::Util::Handle p_Handle)
          : Low::Util::Handle(p_Handle.get_id())
      {
      }

      using Handle::operator=;

      SS2DCanvas &operator=(const SS2DCanvas &) = default;
      SS2DCanvas &operator=(SS2DCanvas &&) noexcept = default;

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static SS2DCanvas *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static SS2DCanvas create_handle_by_index(u32 p_Index);

      static SS2DCanvas find_by_index(uint32_t p_Index);
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

      SS2DCanvas duplicate(Low::Util::Name p_Name) const;
      static SS2DCanvas duplicate(SS2DCanvas p_Handle,
                                  Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static SS2DCanvas find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Serial::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Serial::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        SS2DCanvas l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        SS2DCanvas l_SS2DCanvas = p_Handle.get_id();
        l_SS2DCanvas.destroy();
      }

      Low::Util::List<SS2DDrawCommand> &get_draw_commands() const;

      Low::Renderer::Texture get_out_image() const;
      void set_out_image(Low::Renderer::Texture p_Value);

      Low::Math::UVector2 get_dimensions() const;
      void set_actual_dimensions(Low::Math::UVector2 p_Value);
      void set_actual_dimensions(u32 p_X, u32 p_Y);
      void set_actual_dimensions_x(u32 p_Value);
      void set_actual_dimensions_y(u32 p_Value);

      Low::Math::UVector2 get_desired_dimensions() const;
      void set_dimensions(Low::Math::UVector2 p_Value);
      void set_dimensions(u32 p_X, u32 p_Y);
      void set_dimensions_x(u32 p_Value);
      void set_dimensions_y(u32 p_Value);

      Low::Renderer::Buffer get_drawcommand_index_buffer() const;
      void
      set_drawcommand_index_buffer(Low::Renderer::Buffer p_Value);

      uint64_t get_backend_handle() const;
      void set_backend_handle(uint64_t p_Value);

      bool is_z_dirty() const;
      void set_z_dirty(bool p_Value);
      void toggle_z_dirty();
      void mark_z_dirty();

      bool is_dimensions_dirty() const;
      void set_dimensions_dirty(bool p_Value);
      void toggle_dimensions_dirty();
      void mark_dimensions_dirty();

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static bool get_page_for_index(const u32 p_Index,
                                     u32 &p_PageIndex,
                                     u32 &p_SlotIndex);

    private:
      static u32 ms_Capacity;
      static u32 ms_PageSize;
      static u32 create_instance(u32 &p_PageIndex, u32 &p_SlotIndex);
      static u32 create_page();

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE

      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
