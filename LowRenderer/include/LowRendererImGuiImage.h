#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererBackend.h"
#include "LowRendererImage.h"
#include "LowMathVectorUtil.h"

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
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<ImGuiImage> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        ImGuiImage();
        ImGuiImage(uint64_t p_Id);
        ImGuiImage(ImGuiImage &p_Copy);

      private:
        static ImGuiImage make(Low::Util::Name p_Name);

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

        bool is_alive() const;

        static uint32_t get_capacity();

        static bool is_alive(Low::Util::Handle p_Handle)
        {
          return p_Handle.check_alive(ms_Slots, get_capacity());
        }

        Backend::ImGuiImage &get_imgui_image() const;

        Resource::Image get_image() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static ImGuiImage make(Util::Name p_Name, Resource::Image p_Image);
        void render(Math::UVector2 &p_Dimensions);

      private:
        void set_image(Resource::Image p_Value);
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
