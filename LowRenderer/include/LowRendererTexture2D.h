#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererImage.h"
#include "LowRendererContext.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
namespace Low {
  namespace Util {
    namespace Resource {
      struct Image2D;
    }
  } // namespace Util
} // namespace Low
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER_API Texture2DData
    {
      Resource::Image image;
      Interface::Context context;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(Texture2DData);
      }
    };

    struct LOW_RENDERER_API Texture2D : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Texture2D> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Texture2D();
      Texture2D(uint64_t p_Id);
      Texture2D(Texture2D &p_Copy);

    private:
      static Texture2D make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit Texture2D(const Texture2D &p_Copy)
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
      static Texture2D *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static Texture2D find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      static Texture2D find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle deserialize(Low::Util::Yaml::Node &p_Node,
                                           Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.get_type() == Texture2D::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Texture2D l_Texture2D = p_Handle.get_id();
        l_Texture2D.destroy();
      }

      Resource::Image get_image() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static Texture2D make(Util::Name p_Name, Interface::Context p_Context,
                            Util::Resource::Image2D &p_Image2d);
      static void tick(float p_Delta);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      void set_image(Resource::Image p_Value);
      Interface::Context get_context() const;
      void set_context(Interface::Context p_Value);
    };
  } // namespace Renderer
} // namespace Low
