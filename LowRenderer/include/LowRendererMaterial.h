#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererMaterialType.h"
#include "LowRendererContext.h"
#include "LowUtilVariant.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER_API MaterialData
    {
      MaterialType material_type;
      Interface::Context context;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(MaterialData);
      }
    };

    struct LOW_RENDERER_API Material : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Material> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Material();
      Material(uint64_t p_Id);
      Material(Material &p_Copy);

    private:
      static Material make(Low::Util::Name p_Name);

    public:
      explicit Material(const Material &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static Material *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static Material find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Material l_Material = p_Handle.get_id();
        l_Material.destroy();
      }

      MaterialType get_material_type() const;
      void set_material_type(MaterialType p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static Material make(Util::Name p_Name, Interface::Context p_Context);
      void set_property(Util::Name p_PropertyName, Util::Variant &p_Value);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      Interface::Context get_context() const;
      void set_context(Interface::Context p_Value);
    };
  } // namespace Renderer
} // namespace Low
