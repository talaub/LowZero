#include "LowRendererUniform.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t Uniform::TYPE_ID = 12;
      uint8_t *Uniform::ms_Buffer = 0;
      Low::Util::Instances::Slot *Uniform::ms_Slots = 0;
      Low::Util::List<Uniform> Uniform::ms_LivingInstances =
          Low::Util::List<Uniform>();

      Uniform::Uniform() : Low::Util::Handle(0ull)
      {
      }
      Uniform::Uniform(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      Uniform::Uniform(Uniform &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Uniform Uniform::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        Uniform l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Uniform::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, Uniform, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void Uniform::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        Backend::uniform_cleanup(get_uniform());
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const Uniform *l_Instances = living_instances();
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

      void Uniform::initialize()
      {
        initialize_buffer(&ms_Buffer, UniformData::get_size(), get_capacity(),
                          &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_Uniform);
        LOW_PROFILE_ALLOC(type_slots_Uniform);
      }

      void Uniform::cleanup()
      {
        Low::Util::List<Uniform> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_Uniform);
        LOW_PROFILE_FREE(type_slots_Uniform);
      }

      bool Uniform::is_alive() const
      {
        return m_Data.m_Type == Uniform::TYPE_ID &&
               check_alive(ms_Slots, Uniform::get_capacity());
      }

      uint32_t Uniform::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity =
              Low::Util::Config::get_capacity(N(LowRenderer), N(Uniform));
        }
        return l_Capacity;
      }

      Backend::Uniform &Uniform::get_uniform() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Uniform, uniform, Backend::Uniform);
      }

      Low::Util::Name Uniform::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Uniform, name, Low::Util::Name);
      }
      void Uniform::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Uniform, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      Uniform Uniform::make_buffer(Util::Name p_Name,
                                   UniformBufferCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_buffer
        Uniform l_Uniform = Uniform::make(p_Name);

        Backend::UniformBufferCreateParams l_Params;
        l_Params.context = &(p_Params.context.get_context());
        l_Params.swapchain = &(p_Params.swapchain.get_swapchain());
        l_Params.bufferType = p_Params.bufferType;
        l_Params.bufferSize = p_Params.bufferSize;
        l_Params.binding = p_Params.binding;
        l_Params.arrayIndex = p_Params.arrayIndex;

        Backend::uniform_buffer_create(l_Uniform.get_uniform(), l_Params);

        return l_Uniform;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_buffer
      }

      Uniform Uniform::make_image(Util::Name p_Name,
                                  UniformImageCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_image
        Uniform l_Uniform = Uniform::make(p_Name);

        Backend::UniformImageCreateParams l_Params;
        l_Params.context = &(p_Params.context.get_context());
        l_Params.swapchain = &(p_Params.swapchain.get_swapchain());
        l_Params.imageType = p_Params.imageType;
        l_Params.image = &(p_Params.image.get_image2d());
        l_Params.binding = p_Params.binding;
        l_Params.arrayIndex = p_Params.arrayIndex;

        Backend::uniform_image_create(l_Uniform.get_uniform(), l_Params);

        return l_Uniform;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_image
      }

      void Uniform::set_buffer_initial(void *p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_buffer_initial
        Backend::uniform_buffer_set(get_uniform(), p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_buffer_initial
      }

      void Uniform::set_buffer_frame(UniformBufferSetParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_buffer_frame
        Backend::UniformBufferSetParams l_Params;
        l_Params.context = &(p_Params.context.get_context());
        l_Params.swapchain = &(p_Params.swapchain.get_swapchain());
        l_Params.value = p_Params.value;

        Backend::uniform_buffer_set(get_uniform(), l_Params);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_buffer_frame
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
