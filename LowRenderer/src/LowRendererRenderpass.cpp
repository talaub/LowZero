#include "LowRendererRenderpass.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"
#include "LowRendererImage.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t Renderpass::TYPE_ID = 2;
      uint32_t Renderpass::ms_Capacity = 0u;
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
        uint32_t l_Index = create_instance();

        Renderpass l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Renderpass::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, Renderpass, renderpass,
                                Backend::Renderpass)) Backend::Renderpass();
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
        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowRenderer), N(Renderpass));

        initialize_buffer(&ms_Buffer, RenderpassData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_Renderpass);
        LOW_PROFILE_ALLOC(type_slots_Renderpass);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Renderpass);
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Renderpass::is_alive;
        l_TypeInfo.destroy = &Renderpass::destroy;
        l_TypeInfo.component = false;
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(renderpass);
          l_PropertyInfo.dataOffset = offsetof(RenderpassData, renderpass);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Renderpass, renderpass,
                                              Backend::Renderpass);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Renderpass, renderpass,
                              Backend::Renderpass) =
                *(Backend::Renderpass *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.dataOffset = offsetof(RenderpassData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Renderpass, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Renderpass, name, Low::Util::Name) =
                *(Low::Util::Name *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
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
        return ms_Capacity;
      }

      void Renderpass::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        p_Node["name"] = get_name().c_str();
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
        LOW_ASSERT(p_Params.clearColors.size() == p_Params.renderTargets.size(),
                   "If clear is enabled you have to submit the same amount of "
                   "clearcolors as rendertargets");

        Renderpass l_Renderpass = Renderpass::make(p_Name);

        Backend::RenderpassCreateParams l_Params;
        l_Params.context = &p_Params.context.get_context();
        l_Params.clearDepthColor = p_Params.clearDepthColor;
        l_Params.useDepth = p_Params.useDepth;
        if (p_Params.useDepth) {
          l_Params.depthRenderTarget = &p_Params.depthRenderTarget.get_image();
        }
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

      uint32_t Renderpass::create_instance()
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

      void Renderpass::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

        uint8_t *l_NewBuffer = (uint8_t *)malloc(
            (l_Capacity + l_CapacityIncrease) * sizeof(RenderpassData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(
              &l_NewBuffer[offsetof(RenderpassData, renderpass) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(RenderpassData, renderpass) * (l_Capacity)],
              l_Capacity * sizeof(Backend::Renderpass));
        }
        {
          memcpy(&l_NewBuffer[offsetof(RenderpassData, name) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(RenderpassData, name) * (l_Capacity)],
                 l_Capacity * sizeof(Low::Util::Name));
        }
        for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease;
             ++i) {
          l_NewSlots[i].m_Occupied = false;
          l_NewSlots[i].m_Generation = 0;
        }
        free(ms_Buffer);
        free(ms_Slots);
        ms_Buffer = l_NewBuffer;
        ms_Slots = l_NewSlots;
        ms_Capacity = l_Capacity + l_CapacityIncrease;

        LOW_LOG_DEBUG << "Auto-increased budget for Renderpass from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease) << LOW_LOG_END;
      }
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
