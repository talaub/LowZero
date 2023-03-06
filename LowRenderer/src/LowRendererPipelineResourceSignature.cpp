#include "LowRendererPipelineResourceSignature.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t PipelineResourceSignature::TYPE_ID = 3;
      uint8_t *PipelineResourceSignature::ms_Buffer = 0;
      Low::Util::Instances::Slot *PipelineResourceSignature::ms_Slots = 0;
      Low::Util::List<PipelineResourceSignature>
          PipelineResourceSignature::ms_LivingInstances =
              Low::Util::List<PipelineResourceSignature>();

      PipelineResourceSignature::PipelineResourceSignature()
          : Low::Util::Handle(0ull)
      {
      }
      PipelineResourceSignature::PipelineResourceSignature(uint64_t p_Id)
          : Low::Util::Handle(p_Id)
      {
      }
      PipelineResourceSignature::PipelineResourceSignature(
          PipelineResourceSignature &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      PipelineResourceSignature
      PipelineResourceSignature::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        PipelineResourceSignature l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = PipelineResourceSignature::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, PipelineResourceSignature, name,
                          Low::Util::Name) = Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void PipelineResourceSignature::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const PipelineResourceSignature *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
            ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
            l_LivingInstanceFound = true;
            break;
          }
        }
        _LOW_ASSERT(l_LivingInstanceFound);
      }

      void PipelineResourceSignature::initialize()
      {
        initialize_buffer(&ms_Buffer, PipelineResourceSignatureData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_PipelineResourceSignature);
        LOW_PROFILE_ALLOC(type_slots_PipelineResourceSignature);
      }

      void PipelineResourceSignature::cleanup()
      {
        Low::Util::List<PipelineResourceSignature> l_Instances =
            ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_PipelineResourceSignature);
        LOW_PROFILE_FREE(type_slots_PipelineResourceSignature);
      }

      bool PipelineResourceSignature::is_alive() const
      {
        return m_Data.m_Type == PipelineResourceSignature::TYPE_ID &&
               check_alive(ms_Slots, PipelineResourceSignature::get_capacity());
      }

      uint32_t PipelineResourceSignature::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity = Low::Util::Config::get_capacity(
              N(LowRenderer), N(PipelineResourceSignature));
        }
        return l_Capacity;
      }

      Backend::PipelineResourceSignature &
      PipelineResourceSignature::get_signature() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(PipelineResourceSignature, signature,
                        Backend::PipelineResourceSignature);
      }

      Low::Util::Name PipelineResourceSignature::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(PipelineResourceSignature, name, Low::Util::Name);
      }
      void PipelineResourceSignature::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(PipelineResourceSignature, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      PipelineResourceSignature PipelineResourceSignature::make(
          Util::Name p_Name, Context p_Context, uint8_t p_Binding,
          Util::List<Backend::PipelineResourceDescription>
              &p_ResourceDescriptions)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        PipelineResourceSignature l_Signature =
            PipelineResourceSignature::make(p_Name);

        Backend::PipelineResourceSignatureCreateParams l_Params;
        l_Params.context = &p_Context.get_context();
        l_Params.binding = p_Binding;
        l_Params.resourceDescriptionCount = p_ResourceDescriptions.size();
        l_Params.resourceDescriptions = p_ResourceDescriptions.data();

        Backend::callbacks().pipeline_resource_signature_create(
            l_Signature.get_signature(), l_Params);

        return l_Signature;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      void PipelineResourceSignature::commit()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_commit
        Backend::callbacks().pipeline_resource_signature_commit(
            get_signature());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_commit
      }

      void PipelineResourceSignature::commit_clear()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_commit_clear
        Backend::callbacks().pipeline_resource_signature_commit_clear(
            *get_signature().context);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_commit_clear
      }

      void PipelineResourceSignature::set_image_resource(
          Util::Name p_Name, uint32_t p_ArrayIndex, Resource::Image p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_image_resource
        Backend::callbacks().pipeline_resource_signature_set_image(
            get_signature(), p_Name, p_ArrayIndex, p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_image_resource
      }

      void PipelineResourceSignature::set_sampler_resource(
          Util::Name p_Name, uint32_t p_ArrayIndex, Resource::Image p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_sampler_resource
        Backend::callbacks().pipeline_resource_signature_set_sampler(
            get_signature(), p_Name, p_ArrayIndex, p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_sampler_resource
      }

      void PipelineResourceSignature::set_constant_buffer_resource(
          Util::Name p_Name, uint32_t p_ArrayIndex, Resource::Buffer p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_constant_buffer_resource
        Backend::callbacks().pipeline_resource_signature_set_constant_buffer(
            get_signature(), p_Name, p_ArrayIndex, p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_constant_buffer_resource
      }

      uint8_t PipelineResourceSignature::get_binding()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_binding
        return get_signature().binding;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_binding
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
