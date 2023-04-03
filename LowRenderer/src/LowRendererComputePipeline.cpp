#include "LowRendererComputePipeline.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t ComputePipeline::TYPE_ID = 14;
      uint8_t *ComputePipeline::ms_Buffer = 0;
      Low::Util::Instances::Slot *ComputePipeline::ms_Slots = 0;
      Low::Util::List<ComputePipeline> ComputePipeline::ms_LivingInstances =
          Low::Util::List<ComputePipeline>();

      ComputePipeline::ComputePipeline() : Low::Util::Handle(0ull)
      {
      }
      ComputePipeline::ComputePipeline(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      ComputePipeline::ComputePipeline(ComputePipeline &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      ComputePipeline ComputePipeline::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        ComputePipeline l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = ComputePipeline::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, ComputePipeline, pipeline,
                                Backend::Pipeline)) Backend::Pipeline();
        ACCESSOR_TYPE_SOA(l_Handle, ComputePipeline, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void ComputePipeline::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        PipelineManager::delist_compute_pipeline(*this);
        Backend::callbacks().pipeline_cleanup(get_pipeline());
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const ComputePipeline *l_Instances = living_instances();
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

      void ComputePipeline::initialize()
      {
        initialize_buffer(&ms_Buffer, ComputePipelineData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_ComputePipeline);
        LOW_PROFILE_ALLOC(type_slots_ComputePipeline);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(ComputePipeline);
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &ComputePipeline::is_alive;
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(pipeline);
          l_PropertyInfo.dataOffset = offsetof(ComputePipelineData, pipeline);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ComputePipeline,
                                              pipeline, Backend::Pipeline);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, ComputePipeline, pipeline,
                              Backend::Pipeline) = *(Backend::Pipeline *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.dataOffset = offsetof(ComputePipelineData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ComputePipeline, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, ComputePipeline, name,
                              Low::Util::Name) = *(Low::Util::Name *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void ComputePipeline::cleanup()
      {
        Low::Util::List<ComputePipeline> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_ComputePipeline);
        LOW_PROFILE_FREE(type_slots_ComputePipeline);
      }

      bool ComputePipeline::is_alive() const
      {
        return m_Data.m_Type == ComputePipeline::TYPE_ID &&
               check_alive(ms_Slots, ComputePipeline::get_capacity());
      }

      uint32_t ComputePipeline::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                       N(ComputePipeline));
        }
        return l_Capacity;
      }

      Backend::Pipeline &ComputePipeline::get_pipeline() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(ComputePipeline, pipeline, Backend::Pipeline);
      }

      Low::Util::Name ComputePipeline::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(ComputePipeline, name, Low::Util::Name);
      }
      void ComputePipeline::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(ComputePipeline, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      ComputePipeline
      ComputePipeline::make(Util::Name p_Name,
                            PipelineComputeCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        ComputePipeline l_Pipeline = ComputePipeline::make(p_Name);

        PipelineManager::register_compute_pipeline(l_Pipeline, p_Params);

        return l_Pipeline;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      void ComputePipeline::bind()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_bind
        Backend::callbacks().pipeline_bind(get_pipeline());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_bind
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
