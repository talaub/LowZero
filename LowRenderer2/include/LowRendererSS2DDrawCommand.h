#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    enum class SS2DType
    {
      Circle,
      Rect,
      RoundedRect
    };

    struct SS2DCanvas;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API SS2DDrawCommand
        : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        SS2DType type;
        Low::Math::Vector2 position;
        Low::Math::Vector2 half_extents;
        float rotation;
        Low::Math::Color color;
        Low::Math::Vector4 corner_radius;
        Low::Math::Vector4 uv_rect;
        uint32_t z_sorting;
        bool uploaded;
        uint64_t canvas_handle;
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

      static Low::Util::List<SS2DDrawCommand> ms_LivingInstances;

      const static Low::Util::TypeIdentifier IDENTIFIER;

      [[nodiscard]] static u16 type_id()
      {
        return ms_TypeId;
      }

    private:
      static SS2DDrawCommand make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit SS2DDrawCommand(const SS2DDrawCommand &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      SS2DDrawCommand(u64 p_Id) : Low::Util::Handle(p_Id)
      {
      }
      SS2DDrawCommand() : Low::Util::Handle()
      {
      }
      SS2DDrawCommand(Low::Util::Handle p_Handle)
          : Low::Util::Handle(p_Handle.get_id())
      {
      }

      using Handle::operator=;

      SS2DDrawCommand &operator=(const SS2DDrawCommand &) = default;
      SS2DDrawCommand &
      operator=(SS2DDrawCommand &&) noexcept = default;

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static SS2DDrawCommand *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static SS2DDrawCommand create_handle_by_index(u32 p_Index);

      static SS2DDrawCommand find_by_index(uint32_t p_Index);
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

      SS2DDrawCommand duplicate(Low::Util::Name p_Name) const;
      static SS2DDrawCommand duplicate(SS2DDrawCommand p_Handle,
                                       Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static SS2DDrawCommand find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Serial::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Serial::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        SS2DDrawCommand l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        SS2DDrawCommand l_SS2DDrawCommand = p_Handle.get_id();
        l_SS2DDrawCommand.destroy();
      }

      SS2DType &get_type() const;

      Low::Math::Vector2 get_position() const;
      void set_position(Low::Math::Vector2 p_Value);
      void set_position(float p_X, float p_Y);
      void set_position_x(float p_Value);
      void set_position_y(float p_Value);

      Low::Math::Vector2 get_half_extents() const;
      void set_half_extents(Low::Math::Vector2 p_Value);
      void set_half_extents(float p_X, float p_Y);
      void set_half_extents_x(float p_Value);
      void set_half_extents_y(float p_Value);

      float get_rotation() const;
      void set_rotation(float p_Value);

      Low::Math::Color get_color() const;
      void set_color(Low::Math::Color p_Value);

      Low::Math::Vector4 get_corner_radius() const;
      void set_corner_radius(Low::Math::Vector4 p_Value);

      Low::Math::Vector4 get_uv_rect() const;
      void set_uv_rect(Low::Math::Vector4 p_Value);

      uint32_t get_z_sorting() const;
      void set_z_sorting(uint32_t p_Value);

      bool is_uploaded() const;
      void set_uploaded(bool p_Value);
      void toggle_uploaded();

      uint64_t get_canvas_handle() const;

      void mark_dirty();

      void mark_z_dirty();

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static SS2DDrawCommand make(Low::Util::Name p_Name,
                                  SS2DCanvas p_Canvas,
                                  SS2DType p_Type);
      float get_radius();
      void set_radius(float p_Value);
      static bool get_page_for_index(const u32 p_Index,
                                     u32 &p_PageIndex,
                                     u32 &p_SlotIndex);

    private:
      static u32 ms_Capacity;
      static u32 ms_PageSize;
      static u32 create_instance(u32 &p_PageIndex, u32 &p_SlotIndex);
      static u32 create_page();
      void set_type(SS2DType &p_Value);
      void set_canvas_handle(uint64_t p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE

    public:
      static Low::Util::Set<Low::Renderer::SS2DDrawCommand> ms_Dirty;
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
