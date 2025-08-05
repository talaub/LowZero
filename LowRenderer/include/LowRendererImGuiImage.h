#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererBackend.h"
#include "LowRendererImage.h"
#include "LowMathVectorUtil.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_RENDERER_API ImGuiImageData
      {
        Backend::ImGuiImage imgui_image;
        Resource::Image image;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(ImGuiImageData);
        }
      };

      struct LOW_RENDERER_API ImGuiImage : public Low::Util::Handle
      {
      public:
        static std::shared_mutex ms_BufferMutex;
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<ImGuiImage> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        ImGuiImage();
        ImGuiImage(uint64_t p_Id);
        ImGuiImage(ImGuiImage &p_Copy);

      private:
        static ImGuiImage make(Low::Util::Name p_Name);
        static Low::Util::Handle _make(Low::Util::Name p_Name);

      public:
        explicit ImGuiImage(const ImGuiImage &p_Copy)
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
        static ImGuiImage *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static ImGuiImage find_by_index(uint32_t p_Index);
        static Low::Util::Handle _find_by_index(uint32_t p_Index);

        bool is_alive() const;

        u64 observe(Low::Util::Name p_Observable,
                    Low::Util::Handle p_Observer) const;
        void notify(Low::Util::Handle p_Observed,
                    Low::Util::Name p_Observable);
        void broadcast_observable(Low::Util::Name p_Observable) const;

        static void _notify(Low::Util::Handle p_Observer,
                            Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable);

        static uint32_t get_capacity();

        void serialize(Low::Util::Yaml::Node &p_Node) const;

        ImGuiImage duplicate(Low::Util::Name p_Name) const;
        static ImGuiImage duplicate(ImGuiImage p_Handle,
                                    Low::Util::Name p_Name);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Name p_Name);

        static ImGuiImage find_by_name(Low::Util::Name p_Name);
        static Low::Util::Handle
        _find_by_name(Low::Util::Name p_Name);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Yaml::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          READ_LOCK(l_Lock);
          return p_Handle.get_type() == ImGuiImage::TYPE_ID &&
                 p_Handle.check_alive(ms_Slots, get_capacity());
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          ImGuiImage l_ImGuiImage = p_Handle.get_id();
          l_ImGuiImage.destroy();
        }

        Backend::ImGuiImage &get_imgui_image() const;

        Resource::Image get_image() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static ImGuiImage make(Util::Name p_Name,
                               Resource::Image p_Image);
        void render(Math::UVector2 &p_Dimensions);

      private:
        static uint32_t ms_Capacity;
        static uint32_t create_instance();
        static void increase_budget();
        void set_image(Resource::Image p_Value);

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
