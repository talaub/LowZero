#include "LowRendererGraphicsPipeline.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t GraphicsPipeline::TYPE_ID = 9;
      uint8_t *GraphicsPipeline::ms_Buffer = 0;
      Low::Util::Instances::Slot *GraphicsPipeline::ms_Slots = 0;
      Low::Util::List<GraphicsPipeline> GraphicsPipeline::ms_LivingInstances =
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

      GraphicsPipeline GraphicsPipeline::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        GraphicsPipeline l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = GraphicsPipeline::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, GraphicsPipeline, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void GraphicsPipeline::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        ShaderProgramUtils::delist_graphics_pipeline(*this);

        pipeline_cleanup(get_pipeline());
        // LOW_CODEGEN::END::CUSTOM:DESTROY

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
        _LOW_ASSERT(l_LivingInstanceFound);
      }

      void GraphicsPipeline::initialize()
      {
        initialize_buffer(&ms_Buffer, GraphicsPipelineData::get_size(),
                          get_capacity(), &ms_Slots);
      }

      void GraphicsPipeline::cleanup()
      {
        Low::Util::List<GraphicsPipeline> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);
      }

      bool GraphicsPipeline::is_alive() const
      {
        return m_Data.m_Type == GraphicsPipeline::TYPE_ID &&
               check_alive(ms_Slots, GraphicsPipeline::get_capacity());
      }

      uint32_t GraphicsPipeline::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity = Low::Util::Config::get_capacity(N(GraphicsPipeline));
        }
        return l_Capacity;
      }

      Low::Renderer::Backend::Pipeline &GraphicsPipeline::get_pipeline() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(GraphicsPipeline, pipeline,
                        Low::Renderer::Backend::Pipeline);
      }

      Low::Util::Name GraphicsPipeline::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(GraphicsPipeline, name, Low::Util::Name);
      }
      void GraphicsPipeline::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(GraphicsPipeline, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      GraphicsPipeline
      GraphicsPipeline::make(Util::Name p_Name,
                             GraphicsPipelineCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        GraphicsPipeline l_Pipeline = GraphicsPipeline::make(p_Name);

        ShaderProgramUtils::register_graphics_pipeline(l_Pipeline, p_Params);

        return l_Pipeline;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      void GraphicsPipeline::bind(CommandBuffer p_CommandBuffer)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_bind
        Backend::PipelineBindParams l_Params;
        l_Params.commandBuffer = &(p_CommandBuffer.get_commandbuffer());

        Backend::pipeline_bind(get_pipeline(), l_Params);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_bind
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low