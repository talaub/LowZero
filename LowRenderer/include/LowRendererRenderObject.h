#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowMathVectorUtil.h"
#include "LowRendererMesh.h"
#include "LowRendererMaterial.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_EXPORT RenderObjectData
    {
      Mesh mesh;
      Material material;
      Math::Vector3 world_position;
      Math::Quaternion world_rotation;
      Math::Vector3 world_scale;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(RenderObjectData);
      }
    };

    struct LOW_EXPORT RenderObject : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<RenderObject> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      RenderObject();
      RenderObject(uint64_t p_Id);
      RenderObject(RenderObject &p_Copy);

      static RenderObject make(Low::Util::Name p_Name);
      explicit RenderObject(const RenderObject &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static RenderObject *living_instances()
      {
        return ms_LivingInstances.data();
      }

      bool is_alive() const;

      static uint32_t get_capacity();

      Mesh get_mesh() const;
      void set_mesh(Mesh p_Value);

      Material get_material() const;
      void set_material(Material p_Value);

      Math::Vector3 &get_world_position() const;
      void set_world_position(Math::Vector3 &p_Value);

      Math::Quaternion &get_world_rotation() const;
      void set_world_rotation(Math::Quaternion &p_Value);

      Math::Vector3 &get_world_scale() const;
      void set_world_scale(Math::Vector3 &p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);
    };
  } // namespace Renderer
} // namespace Low
