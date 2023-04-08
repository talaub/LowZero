#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererBackend.h"
#include "LowRendererImage.h"
#include "LowRendererBuffer.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      struct Context;
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_RENDERER_API PipelineResourceSignatureData
      {
        Backend::PipelineResourceSignature signature;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(PipelineResourceSignatureData);
        }
      };

      struct LOW_RENDERER_API PipelineResourceSignature
          : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<PipelineResourceSignature> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        PipelineResourceSignature();
        PipelineResourceSignature(uint64_t p_Id);
        PipelineResourceSignature(PipelineResourceSignature &p_Copy);

      private:
        static PipelineResourceSignature make(Low::Util::Name p_Name);

      public:
        explicit PipelineResourceSignature(
            const PipelineResourceSignature &p_Copy)
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
        static PipelineResourceSignature *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        static bool is_alive(Low::Util::Handle p_Handle)
        {
          return p_Handle.check_alive(ms_Slots, get_capacity());
        }

        Backend::PipelineResourceSignature &get_signature() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static PipelineResourceSignature
        make(Util::Name p_Name, Context p_Context, uint8_t p_Binding,
             Util::List<Backend::PipelineResourceDescription>
                 &p_ResourceDescriptions);
        void commit();
        void set_image_resource(Util::Name p_Name, uint32_t p_ArrayIndex,
                                Resource::Image p_Value);
        void set_sampler_resource(Util::Name p_Name, uint32_t p_ArrayIndex,
                                  Resource::Image p_Value);
        void set_unbound_sampler_resource(Util::Name p_Name,
                                          uint32_t p_ArrayIndex,
                                          Resource::Image p_Value);
        void set_texture2d_resource(Util::Name p_Name, uint32_t p_ArrayIndex,
                                    Resource::Image p_Value);
        void set_constant_buffer_resource(Util::Name p_Name,
                                          uint32_t p_ArrayIndex,
                                          Resource::Buffer p_Value);
        void set_buffer_resource(Util::Name p_Name, uint32_t p_ArrayIndex,
                                 Resource::Buffer p_Value);
        uint8_t get_binding();

      private:
        static uint32_t ms_Capacity;
        static uint32_t create_instance();
        static void increase_budget();
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
