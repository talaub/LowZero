#include "LowRendererUniformScope.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t UniformScope::TYPE_ID = 13;
      uint8_t *UniformScope::ms_Buffer = 0;
      Low::Util::Instances::Slot *UniformScope::ms_Slots = 0;
      Low::Util::List<UniformScope> UniformScope::ms_LivingInstances =
          Low::Util::List<UniformScope>();

      UniformScope::UniformScope() : Low::Util::Handle(0ull)
      {
      }
      UniformScope::UniformScope(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      UniformScope::UniformScope(UniformScope &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      UniformScope UniformScope::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        UniformScope l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = UniformScope::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, UniformScope, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void UniformScope::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        Backend::uniform_scope_cleanup(get_scope());
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const UniformScope *l_Instances = living_instances();
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

      void UniformScope::initialize()
      {
        initialize_buffer(&ms_Buffer, UniformScopeData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_UniformScope);
        LOW_PROFILE_ALLOC(type_slots_UniformScope);
      }

      void UniformScope::cleanup()
      {
        Low::Util::List<UniformScope> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_UniformScope);
        LOW_PROFILE_FREE(type_slots_UniformScope);
      }

      bool UniformScope::is_alive() const
      {
        return m_Data.m_Type == UniformScope::TYPE_ID &&
               check_alive(ms_Slots, UniformScope::get_capacity());
      }

      uint32_t UniformScope::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity =
              Low::Util::Config::get_capacity(N(LowRenderer), N(UniformScope));
        }
        return l_Capacity;
      }

      Backend::UniformScope &UniformScope::get_scope() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(UniformScope, scope, Backend::UniformScope);
      }

      Low::Util::Name UniformScope::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(UniformScope, name, Low::Util::Name);
      }
      void UniformScope::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(UniformScope, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      UniformScope UniformScope::make(Util::Name p_Name,
                                      UniformScopeCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        UniformScope l_Scope = UniformScope::make(p_Name);

        Backend::UniformScopeCreateParams l_Params;
        l_Params.context = &(p_Params.context.get_context());
        l_Params.swapchain = &(p_Params.swapchain.get_swapchain());
        l_Params.pool = &(p_Params.pool.get_pool());
        l_Params.interface = &(p_Params.interface.get_interface());
        l_Params.uniformCount = p_Params.uniforms.size();
        Util::List<Backend::Uniform> l_Uniforms;
        for (uint32_t i = 0u; i < p_Params.uniforms.size(); ++i) {
          l_Uniforms.push_back(p_Params.uniforms[i].get_uniform());
        }
        l_Params.uniforms = l_Uniforms.data();

        Backend::uniform_scope_create(l_Scope.get_scope(), l_Params);

        return l_Scope;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      void UniformScope::bind(UniformScopeBindGraphicsParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_bind
        Backend::UniformScopeBindParams l_Params;
        l_Params.context = &(p_Params.context.get_context());
        l_Params.swapchain = &(p_Params.swapchain.get_swapchain());
        l_Params.pipeline = &(p_Params.pipeline.get_pipeline());
        l_Params.startIndex = p_Params.startIndex;
        l_Params.scopeCount = static_cast<uint32_t>(p_Params.scopes.size());
        Util::List<Backend::UniformScope> l_Scopes;
        for (uint32_t i = 0u; i < l_Params.scopeCount; ++i) {
          l_Scopes.push_back(p_Params.scopes[i].get_scope());
        }
        l_Params.scopes = l_Scopes.data();

        Backend::uniform_scopes_bind(l_Params);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_bind
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
