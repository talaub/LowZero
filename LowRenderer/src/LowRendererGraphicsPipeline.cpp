#include "LowRendererGraphicsPipeline.h"

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

      const uint16_t GraphicsPipeline::TYPE_ID = 5;
      uint32_t GraphicsPipeline::ms_Capacity = 0u;
      uint8_t *GraphicsPipeline::ms_Buffer = 0;
      std::shared_mutex GraphicsPipeline::ms_BufferMutex;
      Low::Util::Instances::Slot *GraphicsPipeline::ms_Slots = 0;
      Low::Util::List<GraphicsPipeline>
          GraphicsPipeline::ms_LivingInstances =
              Low::Util::List<GraphicsPipeline>();

      GraphicsPipeline::GraphicsPipeline() : Low::Util::Handle(0ull)
      {
      }
      GraphicsPipeline::GraphicsPipeline(uint64_t p_Id)
          : Low::Util::Handle(p_Id)
      {
      }
      GraphicsPipeline::GraphicsPipeline(GraphicsPipeline &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle
      GraphicsPipeline::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      GraphicsPipeline GraphicsPipeline::make(Low::Util::Name p_Name)
      {
        WRITE_LOCK(l_Lock);
        uint32_t l_Index = create_instance();

        GraphicsPipeline l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = GraphicsPipeline::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, GraphicsPipeline, pipeline,
                                Backend::Pipeline))
            Backend::Pipeline();
        ACCESSOR_TYPE_SOA(l_Handle, GraphicsPipeline, name,
                          Low::Util::Name) = Low::Util::Name(0u);
        LOCK_UNLOCK(l_Lock);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void GraphicsPipeline::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

        PipelineManager::delist_graphics_pipeline(*this);
        Backend::callbacks().pipeline_cleanup(get_pipeline());
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        WRITE_LOCK(l_Lock);
        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const GraphicsPipeline *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
            ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
            l_LivingInstanceFound = true;
            break;
          }
        }
      }

      void GraphicsPipeline::initialize()
      {
        WRITE_LOCK(l_Lock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(
            N(LowRenderer), N(GraphicsPipeline));

        initialize_buffer(&ms_Buffer,
                          GraphicsPipelineData::get_size(),
                          get_capacity(), &ms_Slots);
        LOCK_UNLOCK(l_Lock);

        LOW_PROFILE_ALLOC(type_buffer_GraphicsPipeline);
        LOW_PROFILE_ALLOC(type_slots_GraphicsPipeline);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(GraphicsPipeline);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &GraphicsPipeline::is_alive;
        l_TypeInfo.destroy = &GraphicsPipeline::destroy;
        l_TypeInfo.serialize = &GraphicsPipeline::serialize;
        l_TypeInfo.deserialize = &GraphicsPipeline::deserialize;
        l_TypeInfo.find_by_index = &GraphicsPipeline::_find_by_index;
        l_TypeInfo.find_by_name = &GraphicsPipeline::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &GraphicsPipeline::_make;
        l_TypeInfo.duplicate_default = &GraphicsPipeline::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &GraphicsPipeline::living_instances);
        l_TypeInfo.get_living_count = &GraphicsPipeline::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: pipeline
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(pipeline);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(GraphicsPipelineData, pipeline);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            GraphicsPipeline l_Handle = p_Handle.get_id();
            l_Handle.get_pipeline();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, GraphicsPipeline, pipeline,
                Backend::Pipeline);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            GraphicsPipeline l_Handle = p_Handle.get_id();
            *((Backend::Pipeline *)p_Data) = l_Handle.get_pipeline();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: pipeline
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(GraphicsPipelineData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            GraphicsPipeline l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, GraphicsPipeline, name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            GraphicsPipeline l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            GraphicsPipeline l_Handle = p_Handle.get_id();
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Function: make
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(make);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType = GraphicsPipeline::TYPE_ID;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Name);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::NAME;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Params);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: make
        }
        {
          // Function: bind
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(bind);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: bind
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void GraphicsPipeline::cleanup()
      {
        Low::Util::List<GraphicsPipeline> l_Instances =
            ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        WRITE_LOCK(l_Lock);
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_GraphicsPipeline);
        LOW_PROFILE_FREE(type_slots_GraphicsPipeline);
        LOCK_UNLOCK(l_Lock);
      }

      Low::Util::Handle
      GraphicsPipeline::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      GraphicsPipeline
      GraphicsPipeline::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        GraphicsPipeline l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = GraphicsPipeline::TYPE_ID;

        return l_Handle;
      }

      bool GraphicsPipeline::is_alive() const
      {
        READ_LOCK(l_Lock);
        return m_Data.m_Type == GraphicsPipeline::TYPE_ID &&
               check_alive(ms_Slots,
                           GraphicsPipeline::get_capacity());
      }

      uint32_t GraphicsPipeline::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      GraphicsPipeline::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      GraphicsPipeline
      GraphicsPipeline::find_by_name(Low::Util::Name p_Name)
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
        return 0ull;
      }

      GraphicsPipeline
      GraphicsPipeline::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        GraphicsPipeline l_Handle = make(p_Name);

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      GraphicsPipeline
      GraphicsPipeline::duplicate(GraphicsPipeline p_Handle,
                                  Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      GraphicsPipeline::_duplicate(Low::Util::Handle p_Handle,
                                   Low::Util::Name p_Name)
      {
        GraphicsPipeline l_GraphicsPipeline = p_Handle.get_id();
        return l_GraphicsPipeline.duplicate(p_Name);
      }

      void
      GraphicsPipeline::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void GraphicsPipeline::serialize(Low::Util::Handle p_Handle,
                                       Low::Util::Yaml::Node &p_Node)
      {
        GraphicsPipeline l_GraphicsPipeline = p_Handle.get_id();
        l_GraphicsPipeline.serialize(p_Node);
      }

      Low::Util::Handle
      GraphicsPipeline::deserialize(Low::Util::Yaml::Node &p_Node,
                                    Low::Util::Handle p_Creator)
      {
        GraphicsPipeline l_Handle =
            GraphicsPipeline::make(N(GraphicsPipeline));

        if (p_Node["pipeline"]) {
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      Backend::Pipeline &GraphicsPipeline::get_pipeline() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pipeline

        // LOW_CODEGEN::END::CUSTOM:GETTER_pipeline

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(GraphicsPipeline, pipeline,
                        Backend::Pipeline);
      }

      Low::Util::Name GraphicsPipeline::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(GraphicsPipeline, name, Low::Util::Name);
      }
      void GraphicsPipeline::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(GraphicsPipeline, name, Low::Util::Name) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      GraphicsPipeline
      GraphicsPipeline::make(Util::Name p_Name,
                             PipelineGraphicsCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make

        GraphicsPipeline l_Pipeline = GraphicsPipeline::make(p_Name);

        PipelineManager::register_graphics_pipeline(l_Pipeline,
                                                    p_Params);

        return l_Pipeline;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      void GraphicsPipeline::bind()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_bind

        Backend::callbacks().pipeline_bind(get_pipeline());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_bind
      }

      uint32_t GraphicsPipeline::create_instance()
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

      void GraphicsPipeline::increase_budget()
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
                              sizeof(GraphicsPipelineData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(
              &l_NewBuffer[offsetof(GraphicsPipelineData, pipeline) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(GraphicsPipelineData, pipeline) *
                         (l_Capacity)],
              l_Capacity * sizeof(Backend::Pipeline));
        }
        {
          memcpy(&l_NewBuffer[offsetof(GraphicsPipelineData, name) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(GraphicsPipelineData, name) *
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
            << "Auto-increased budget for GraphicsPipeline from "
            << l_Capacity << " to "
            << (l_Capacity + l_CapacityIncrease) << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
