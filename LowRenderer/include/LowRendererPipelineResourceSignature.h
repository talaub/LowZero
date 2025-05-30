#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererBackend.h"
#include "LowRendererImage.h"
#include "LowRendererBuffer.h"

#include "shared_mutex"
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
        static std::shared_mutex ms_BufferMutex;
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<PipelineResourceSignature>
            ms_LivingInstances;

        const static uint16_t TYPE_ID;

        PipelineResourceSignature();
        PipelineResourceSignature(uint64_t p_Id);
        PipelineResourceSignature(PipelineResourceSignature &p_Copy);

      private:
        static PipelineResourceSignature make(Low::Util::Name p_Name);
        static Low::Util::Handle _make(Low::Util::Name p_Name);

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

        static PipelineResourceSignature
        find_by_index(uint32_t p_Index);
        static Low::Util::Handle _find_by_index(uint32_t p_Index);

        bool is_alive() const;

        static uint32_t get_capacity();

        void serialize(Low::Util::Yaml::Node &p_Node) const;

        PipelineResourceSignature
        duplicate(Low::Util::Name p_Name) const;
        static PipelineResourceSignature
        duplicate(PipelineResourceSignature p_Handle,
                  Low::Util::Name p_Name);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Name p_Name);

        static PipelineResourceSignature
        find_by_name(Low::Util::Name p_Name);
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
          return p_Handle.get_type() ==
                     PipelineResourceSignature::TYPE_ID &&
                 p_Handle.check_alive(ms_Slots, get_capacity());
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          PipelineResourceSignature l_PipelineResourceSignature =
              p_Handle.get_id();
          l_PipelineResourceSignature.destroy();
        }

        Backend::PipelineResourceSignature &get_signature() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static PipelineResourceSignature
        make(Util::Name p_Name, Context p_Context, uint8_t p_Binding,
             Util::List<Backend::PipelineResourceDescription>
                 &p_ResourceDescriptions);
        void commit();
        void set_image_resource(Util::Name p_Name,
                                uint32_t p_ArrayIndex,
                                Resource::Image p_Value);
        void set_sampler_resource(Util::Name p_Name,
                                  uint32_t p_ArrayIndex,
                                  Resource::Image p_Value);
        void set_unbound_sampler_resource(Util::Name p_Name,
                                          uint32_t p_ArrayIndex,
                                          Resource::Image p_Value);
        void set_texture2d_resource(Util::Name p_Name,
                                    uint32_t p_ArrayIndex,
                                    Resource::Image p_Value);
        void set_constant_buffer_resource(Util::Name p_Name,
                                          uint32_t p_ArrayIndex,
                                          Resource::Buffer p_Value);
        void set_buffer_resource(Util::Name p_Name,
                                 uint32_t p_ArrayIndex,
                                 Resource::Buffer p_Value);
        uint8_t get_binding();

      private:
        static uint32_t ms_Capacity;
        static uint32_t create_instance();
        static void increase_budget();

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
