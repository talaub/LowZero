#include "LowRendererVkPipeline.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t Pipeline::TYPE_ID = 45;
      uint32_t Pipeline::ms_Capacity = 0u;
      uint8_t *Pipeline::ms_Buffer = 0;
      Low::Util::Instances::Slot *Pipeline::ms_Slots = 0;
      Low::Util::List<Pipeline> Pipeline::ms_LivingInstances =
          Low::Util::List<Pipeline>();

      Pipeline::Pipeline() : Low::Util::Handle(0ull)
      {
      }
      Pipeline::Pipeline(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      Pipeline::Pipeline(Pipeline &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle Pipeline::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Pipeline Pipeline::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = create_instance();

        Pipeline l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Pipeline::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, Pipeline, pipeline,
                                VkPipeline)) VkPipeline();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Pipeline, layout,
                                VkPipelineLayout)) VkPipelineLayout();
        ACCESSOR_TYPE_SOA(l_Handle, Pipeline, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Pipeline::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        vkDestroyPipelineLayout(get_context()->device, get_layout(),
                                nullptr);
        vkDestroyPipeline(get_context()->device, get_pipeline(),
                          nullptr);
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const Pipeline *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
            ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
            l_LivingInstanceFound = true;
            break;
          }
        }
      }

      void Pipeline::initialize()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                      N(Pipeline));

        initialize_buffer(&ms_Buffer, PipelineData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_Pipeline);
        LOW_PROFILE_ALLOC(type_slots_Pipeline);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Pipeline);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Pipeline::is_alive;
        l_TypeInfo.destroy = &Pipeline::destroy;
        l_TypeInfo.serialize = &Pipeline::serialize;
        l_TypeInfo.deserialize = &Pipeline::deserialize;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Pipeline::_make;
        l_TypeInfo.duplicate_default = &Pipeline::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Pipeline::living_instances);
        l_TypeInfo.get_living_count = &Pipeline::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(pipeline);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(PipelineData, pipeline);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Pipeline l_Handle = p_Handle.get_id();
            l_Handle.get_pipeline();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Pipeline,
                                              pipeline, VkPipeline);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Pipeline l_Handle = p_Handle.get_id();
            l_Handle.set_pipeline(*(VkPipeline *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(layout);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(PipelineData, layout);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Pipeline l_Handle = p_Handle.get_id();
            l_Handle.get_layout();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Pipeline, layout, VkPipelineLayout);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Pipeline l_Handle = p_Handle.get_id();
            l_Handle.set_layout(*(VkPipelineLayout *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(context);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(PipelineData, context);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            return nullptr;
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(PipelineData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Pipeline l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Pipeline,
                                              name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Pipeline l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(make);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType = Pipeline::TYPE_ID;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Context);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void Pipeline::cleanup()
      {
        Low::Util::List<Pipeline> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_Pipeline);
        LOW_PROFILE_FREE(type_slots_Pipeline);
      }

      Pipeline Pipeline::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Pipeline l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = Pipeline::TYPE_ID;

        return l_Handle;
      }

      bool Pipeline::is_alive() const
      {
        return m_Data.m_Type == Pipeline::TYPE_ID &&
               check_alive(ms_Slots, Pipeline::get_capacity());
      }

      uint32_t Pipeline::get_capacity()
      {
        return ms_Capacity;
      }

      Pipeline Pipeline::find_by_name(Low::Util::Name p_Name)
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
      }

      Pipeline Pipeline::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        Pipeline l_Handle = make(p_Name);
        l_Handle.set_pipeline(get_pipeline());
        l_Handle.set_layout(get_layout());
        l_Handle.set_context(get_context());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      Pipeline Pipeline::duplicate(Pipeline p_Handle,
                                   Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      Pipeline::_duplicate(Low::Util::Handle p_Handle,
                           Low::Util::Name p_Name)
      {
        Pipeline l_Pipeline = p_Handle.get_id();
        return l_Pipeline.duplicate(p_Name);
      }

      void Pipeline::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Pipeline::serialize(Low::Util::Handle p_Handle,
                               Low::Util::Yaml::Node &p_Node)
      {
        Pipeline l_Pipeline = p_Handle.get_id();
        l_Pipeline.serialize(p_Node);
      }

      Low::Util::Handle
      Pipeline::deserialize(Low::Util::Yaml::Node &p_Node,
                            Low::Util::Handle p_Creator)
      {
        Pipeline l_Handle = Pipeline::make(N(Pipeline));

        if (p_Node["pipeline"]) {
        }
        if (p_Node["layout"]) {
        }
        if (p_Node["context"]) {
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      VkPipeline &Pipeline::get_pipeline() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pipeline
        // LOW_CODEGEN::END::CUSTOM:GETTER_pipeline

        return TYPE_SOA(Pipeline, pipeline, VkPipeline);
      }
      void Pipeline::set_pipeline(VkPipeline &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_pipeline
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_pipeline

        // Set new value
        TYPE_SOA(Pipeline, pipeline, VkPipeline) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_pipeline
        // LOW_CODEGEN::END::CUSTOM:SETTER_pipeline
      }

      VkPipelineLayout &Pipeline::get_layout() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_layout
        // LOW_CODEGEN::END::CUSTOM:GETTER_layout

        return TYPE_SOA(Pipeline, layout, VkPipelineLayout);
      }
      void Pipeline::set_layout(VkPipelineLayout &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_layout
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_layout

        // Set new value
        TYPE_SOA(Pipeline, layout, VkPipelineLayout) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_layout
        // LOW_CODEGEN::END::CUSTOM:SETTER_layout
      }

      Context *Pipeline::get_context() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_context
        // LOW_CODEGEN::END::CUSTOM:GETTER_context

        return TYPE_SOA(Pipeline, context, Context *);
      }
      void Pipeline::set_context(Context *p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_context
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_context

        // Set new value
        TYPE_SOA(Pipeline, context, Context *) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_context
        // LOW_CODEGEN::END::CUSTOM:SETTER_context
      }

      Low::Util::Name Pipeline::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Pipeline, name, Low::Util::Name);
      }
      void Pipeline::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(Pipeline, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      Pipeline Pipeline::make(Context &p_Context)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        Pipeline l_Handle = Pipeline::make(N(Pipeline));
        l_Handle.set_context(&p_Context);

        return l_Handle;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      uint32_t Pipeline::create_instance()
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

      void Pipeline::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease =
            std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0,
                   "Could not increase capacity");

        uint8_t *l_NewBuffer = (uint8_t *)malloc(
            (l_Capacity + l_CapacityIncrease) * sizeof(PipelineData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(&l_NewBuffer[offsetof(PipelineData, pipeline) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(PipelineData, pipeline) *
                            (l_Capacity)],
                 l_Capacity * sizeof(VkPipeline));
        }
        {
          memcpy(&l_NewBuffer[offsetof(PipelineData, layout) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(PipelineData, layout) *
                            (l_Capacity)],
                 l_Capacity * sizeof(VkPipelineLayout));
        }
        {
          memcpy(&l_NewBuffer[offsetof(PipelineData, context) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(PipelineData, context) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Context *));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(PipelineData, name) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(PipelineData, name) * (l_Capacity)],
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

        LOW_LOG_DEBUG << "Auto-increased budget for Pipeline from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease)
                      << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
