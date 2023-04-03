#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER_API MeshData
    {
      uint32_t vertex_buffer_start;
      uint32_t vertex_count;
      uint32_t index_buffer_start;
      uint32_t index_count;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(MeshData);
      }
    };

    struct LOW_RENDERER_API Mesh : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Mesh> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Mesh();
      Mesh(uint64_t p_Id);
      Mesh(Mesh &p_Copy);

      static Mesh make(Low::Util::Name p_Name);
      explicit Mesh(const Mesh &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static Mesh *living_instances()
      {
        return ms_LivingInstances.data();
      }

      bool is_alive() const;

      static uint32_t get_capacity();

      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.check_alive(ms_Slots, get_capacity());
      }

      uint32_t get_vertex_buffer_start() const;
      void set_vertex_buffer_start(uint32_t p_Value);

      uint32_t get_vertex_count() const;
      void set_vertex_count(uint32_t p_Value);

      uint32_t get_index_buffer_start() const;
      void set_index_buffer_start(uint32_t p_Value);

      uint32_t get_index_count() const;
      void set_index_count(uint32_t p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);
    };
  } // namespace Renderer
} // namespace Low
