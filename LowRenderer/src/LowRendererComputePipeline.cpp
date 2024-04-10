#include "LowRendererComputePipeline.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowRendererInterface.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t ComputePipeline::TYPE_ID = 4;
      uint32_t ComputePipeline::ms_Capacity = 0u;
      uint8_t *ComputePipeline::ms_Buffer = 0;
      Low::Util::Instances::Slot *ComputePipeline::ms_Slots = 0;
      Low::Util::List<ComputePipeline>
          ComputePipeline::ms_LivingInstances =
              Low::Util::List<ComputePipeline>();

      ComputePipeline::ComputePipeline() : Low::Util::Handle(0ull)
      {
      }
      ComputePipeline::ComputePipeline(uint64_t p_Id)
          : Low::Util::Handle(p_Id)
      {
      }
      ComputePipeline::ComputePipeline(ComputePipeline &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle ComputePipeline::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      ComputePipeline ComputePipeline::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = create_instance();

        ComputePipeline l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = ComputePipeline::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, ComputePipeline, pipeline,
                                Backend::Pipeline))
            Backend::Pipeline();
        ACCESSOR_TYPE_SOA(l_Handle, ComputePipeline, name,
                          Low::Util::Name) = Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        // LOW_CODEGEN::END::CUSTOM:MAKE

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
      }

      void ComputePipeline::initialize()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(
            N(LowRenderer), N(ComputePipeline));

        initialize_buffer(&ms_Buffer, ComputePipelineData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_ComputePipeline);
        LOW_PROFILE_ALLOC(type_slots_ComputePipeline);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(ComputePipeline);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &ComputePipeline::is_alive;
        l_TypeInfo.destroy = &ComputePipeline::destroy;
        l_TypeInfo.serialize = &ComputePipeline::serialize;
        l_TypeInfo.deserialize = &ComputePipeline::deserialize;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &ComputePipeline::_make;
        l_TypeInfo.duplicate_default = &ComputePipeline::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &ComputePipeline::living_instances);
        l_TypeInfo.get_living_count = &ComputePipeline::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(pipeline);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ComputePipelineData, pipeline);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            ComputePipeline l_Handle = p_Handle.get_id();
            l_Handle.get_pipeline();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, ComputePipeline, pipeline,
                Backend::Pipeline);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ComputePipelineData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            ComputePipeline l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, ComputePipeline, name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ComputePipeline l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void ComputePipeline::cleanup()
      {
        Low::Util::List<ComputePipeline> l_Instances =
            ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_ComputePipeline);
        LOW_PROFILE_FREE(type_slots_ComputePipeline);
      }

      ComputePipeline ComputePipeline::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        ComputePipeline l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = ComputePipeline::TYPE_ID;

        return l_Handle;
      }

      bool ComputePipeline::is_alive() const
      {
        return m_Data.m_Type == ComputePipeline::TYPE_ID &&
               check_alive(ms_Slots, ComputePipeline::get_capacity());
      }

      uint32_t ComputePipeline::get_capacity()
      {
        return ms_Capacity;
      }

      ComputePipeline
      ComputePipeline::find_by_name(Low::Util::Name p_Name)
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
      }

      ComputePipeline
      ComputePipeline::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        ComputePipeline l_Handle = make(p_Name);

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      ComputePipeline
      ComputePipeline::duplicate(ComputePipeline p_Handle,
                                 Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      ComputePipeline::_duplicate(Low::Util::Handle p_Handle,
                                  Low::Util::Name p_Name)
      {
        ComputePipeline l_ComputePipeline = p_Handle.get_id();
        return l_ComputePipeline.duplicate(p_Name);
      }

      void
      ComputePipeline::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void ComputePipeline::serialize(Low::Util::Handle p_Handle,
                                      Low::Util::Yaml::Node &p_Node)
      {
        ComputePipeline l_ComputePipeline = p_Handle.get_id();
        l_ComputePipeline.serialize(p_Node);
      }

      Low::Util::Handle
      ComputePipeline::deserialize(Low::Util::Yaml::Node &p_Node,
                                   Low::Util::Handle p_Creator)
      {
        ComputePipeline l_Handle =
            ComputePipeline::make(N(ComputePipeline));

        if (p_Node["pipeline"]) {
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      Backend::Pipeline &ComputePipeline::get_pipeline() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pipeline
        // LOW_CODEGEN::END::CUSTOM:GETTER_pipeline

        return TYPE_SOA(ComputePipeline, pipeline, Backend::Pipeline);
      }

      Low::Util::Name ComputePipeline::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(ComputePipeline, name, Low::Util::Name);
      }
      void ComputePipeline::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

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

        PipelineManager::register_compute_pipeline(l_Pipeline,
                                                   p_Params);

        return l_Pipeline;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      void ComputePipeline::bind()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_bind
        Backend::callbacks().pipeline_bind(get_pipeline());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_bind
      }

      void ComputePipeline::set_constant(Util::Name p_Name,
                                         void *p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_constant
        Backend::callbacks().pipeline_set_constant(get_pipeline(),
                                                   p_Name, p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_constant
      }

      uint32_t ComputePipeline::create_instance()
      {
        uint32_t l_Index = 0u;

        for (; l_Index < get_capacity(); ++l_Index) {
          if (!ms_Slots[l_Index].m_Occupied) {
            break;
          }
        }
        if (l_Index >= get_capacity()) {
          increase_budget();
        }
        ms_Slots[l_Index].m_Occupied = true;
        return l_Index;
      }

      void ComputePipeline::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease =
            std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0,
                   "Could not increase capacity");

        uint8_t *l_NewBuffer =
            (uint8_t *)malloc((l_Capacity + l_CapacityIncrease) *
                              sizeof(ComputePipelineData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(
              &l_NewBuffer[offsetof(ComputePipelineData, pipeline) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(ComputePipelineData, pipeline) *
                         (l_Capacity)],
              l_Capacity * sizeof(Backend::Pipeline));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ComputePipelineData, name) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ComputePipelineData, name) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Util::Name));
        }
        for (uint32_t i = l_Capacity;
             i < l_Capacity + l_CapacityIncrease; ++i) {
          l_NewSlots[i].m_Occupied = false;
          l_NewSlots[i].m_Generation = 0;
        }
        free(ms_Buffer);
        free(ms_Slots);
        ms_Buffer = l_NewBuffer;
        ms_Slots = l_NewSlots;
        ms_Capacity = l_Capacity + l_CapacityIncrease;

        LOW_LOG_DEBUG
            << "Auto-increased budget for ComputePipeline from "
            << l_Capacity << " to "
            << (l_Capacity + l_CapacityIncrease) << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
