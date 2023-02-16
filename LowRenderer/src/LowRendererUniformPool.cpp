#include "LowRendererUniformPool.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t UniformPool::TYPE_ID = 13;
      uint8_t *UniformPool::ms_Buffer = 0;
      Low::Util::Instances::Slot *UniformPool::ms_Slots = 0;
      Low::Util::List<UniformPool> UniformPool::ms_LivingInstances =
          Low::Util::List<UniformPool>();

      UniformPool::UniformPool() : Low::Util::Handle(0ull)
      {
      }
      UniformPool::UniformPool(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      UniformPool::UniformPool(UniformPool &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      UniformPool UniformPool::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        UniformPool l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = UniformPool::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, UniformPool, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void UniformPool::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        Backend::uniform_pool_cleanup(get_pool());
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const UniformPool *l_Instances = living_instances();
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

      void UniformPool::initialize()
      {
        initialize_buffer(&ms_Buffer, UniformPoolData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_UniformPool);
        LOW_PROFILE_ALLOC(type_slots_UniformPool);
      }

      void UniformPool::cleanup()
      {
        Low::Util::List<UniformPool> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_UniformPool);
        LOW_PROFILE_FREE(type_slots_UniformPool);
      }

      bool UniformPool::is_alive() const
      {
        return m_Data.m_Type == UniformPool::TYPE_ID &&
               check_alive(ms_Slots, UniformPool::get_capacity());
      }

      uint32_t UniformPool::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity =
              Low::Util::Config::get_capacity(N(LowRenderer), N(UniformPool));
        }
        return l_Capacity;
      }

      Backend::UniformPool &UniformPool::get_pool() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(UniformPool, pool, Backend::UniformPool);
      }

      Low::Util::Name UniformPool::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(UniformPool, name, Low::Util::Name);
      }
      void UniformPool::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(UniformPool, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      UniformPool UniformPool::make(Util::Name p_Name,
                                    UniformPoolCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        UniformPool l_Pool = UniformPool::make(p_Name);

        Backend::UniformPoolCreateParams l_Params;
        l_Params.context = &(p_Params.context.get_context());
        l_Params.scopeCount = p_Params.scopeCount;
        l_Params.rendertargetCount = p_Params.rendertargetCount;
        l_Params.samplerCount = p_Params.samplerCount;
        l_Params.storageBufferCount = p_Params.storageBufferCount;
        l_Params.uniformBufferCount = p_Params.uniformBufferCount;

        Backend::uniform_pool_create(l_Pool.get_pool(), l_Params);

        return l_Pool;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
