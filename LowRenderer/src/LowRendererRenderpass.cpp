#include "LowRendererRenderpass.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"
#include "LowRendererImage.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t Renderpass::TYPE_ID = 12;
      uint8_t *Renderpass::ms_Buffer = 0;
      Low::Util::Instances::Slot *Renderpass::ms_Slots = 0;
      Low::Util::List<Renderpass> Renderpass::ms_LivingInstances =
          Low::Util::List<Renderpass>();

      Renderpass::Renderpass() : Low::Util::Handle(0ull)
      {
      }
      Renderpass::Renderpass(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      Renderpass::Renderpass(Renderpass &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Renderpass Renderpass::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        RenderpassData *l_DataPtr =
            (RenderpassData *)&ms_Buffer[l_Index * sizeof(RenderpassData)];
        new (l_DataPtr) RenderpassData();

        Renderpass l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Renderpass::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, Renderpass, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void Renderpass::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        if (!get_renderpass().swapchainRenderpass) {
          Backend::callbacks().renderpass_cleanup(get_renderpass());
        }
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const Renderpass *l_Instances = living_instances();
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

      void Renderpass::initialize()
      {
        initialize_buffer(&ms_Buffer, RenderpassData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_Renderpass);
        LOW_PROFILE_ALLOC(type_slots_Renderpass);
      }

      void Renderpass::cleanup()
      {
        Low::Util::List<Renderpass> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_Renderpass);
        LOW_PROFILE_FREE(type_slots_Renderpass);
      }

      bool Renderpass::is_alive() const
      {
        return m_Data.m_Type == Renderpass::TYPE_ID &&
               check_alive(ms_Slots, Renderpass::get_capacity());
      }

      uint32_t Renderpass::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity =
              Low::Util::Config::get_capacity(N(LowRenderer), N(Renderpass));
        }
        return l_Capacity;
      }

      Backend::Renderpass &Renderpass::get_renderpass() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Renderpass, renderpass, Backend::Renderpass);
      }
      void Renderpass::set_renderpass(Backend::Renderpass &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Renderpass, renderpass, Backend::Renderpass) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_renderpass
        // LOW_CODEGEN::END::CUSTOM:SETTER_renderpass
      }

      Low::Util::Name Renderpass::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Renderpass, name, Low::Util::Name);
      }
      void Renderpass::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Renderpass, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      Renderpass Renderpass::make(Util::Name p_Name,
                                  RenderpassCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        LOW_ASSERT(!p_Params.clearColors.empty() &&
                       p_Params.clearColors.size() ==
                           p_Params.renderTargets.size(),
                   "If clear is enabled you have to submit the same amount of "
                   "clearcolors as rendertargets");

        Renderpass l_Renderpass = Renderpass::make(p_Name);

        Backend::RenderpassCreateParams l_Params;
        l_Params.context = &p_Params.context.get_context();
        l_Params.clearDepthColor = p_Params.clearDepthColor;
        l_Params.useDepth = p_Params.useDepth;
        l_Params.dimensions = p_Params.dimensions;
        l_Params.clearTargetColor = p_Params.clearColors.data();
        l_Params.renderTargetCount =
            static_cast<uint8_t>(p_Params.renderTargets.size());

        Util::List<Backend::ImageResource> l_TargetResources;
        for (uint8_t i = 0u; i < l_Params.renderTargetCount; ++i) {
          l_TargetResources.push_back(p_Params.renderTargets[i].get_image());
        }
        l_Params.renderTargets = l_TargetResources.data();

        Backend::callbacks().renderpass_create(l_Renderpass.get_renderpass(),
                                               l_Params);

        return l_Renderpass;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      Math::UVector2 &Renderpass::get_dimensions()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_dimensions
        return get_renderpass().dimensions;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_dimensions
      }

      void Renderpass::begin()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_begin
        Backend::callbacks().renderpass_begin(get_renderpass());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_begin
      }

      void Renderpass::end()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_end
        Backend::callbacks().renderpass_end(get_renderpass());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_end
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
