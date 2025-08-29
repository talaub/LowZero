#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#define RENDERSTEP_SOLID_MATERIAL_NAME N(solid_material)
#define RENDERSTEP_LIGHTING_NAME N(phong_lighting)
#define RENDERSTEP_LIGHTCULLING_NAME N(basic_light_culling)
#define RENDERSTEP_SSAO_NAME N(basic_ssao)
#define RENDERSTEP_CAVITIES_NAME N(cavities)
#define RENDERSTEP_UI_NAME N(ui)
#define RENDERSTEP_DEBUG_GEOMETRY_NAME N(debug_geometry)
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    struct RenderView;
    struct RenderStep;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API RenderStepData
    {
      Low::Util::Function<bool(RenderStep)> setup_callback;
      Low::Util::Function<bool(Low::Renderer::RenderStep,
                               Low::Renderer::RenderView)>
          prepare_callback;
      Low::Util::Function<bool(Low::Renderer::RenderStep,
                               Low::Renderer::RenderView)>
          teardown_callback;
      Low::Util::Function<bool(Low::Renderer::RenderStep, float,
                               Low::Renderer::RenderView)>
          execute_callback;
      Low::Util::Function<bool(Low::Renderer::RenderStep,
                               Low::Math::UVector2 p_NewDimensions,
                               Low::Renderer::RenderView)>
          resolution_update_callback;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(RenderStepData);
      }
    };

    struct LOW_RENDERER2_API RenderStep : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<RenderStep> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      RenderStep();
      RenderStep(uint64_t p_Id);
      RenderStep(RenderStep &p_Copy);

      static RenderStep make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit RenderStep(const RenderStep &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static RenderStep *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static RenderStep find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      u64 observe(Low::Util::Name p_Observable,
                  Low::Util::Handle p_Observer) const;
      void notify(Low::Util::Handle p_Observed,
                  Low::Util::Name p_Observable);
      void broadcast_observable(Low::Util::Name p_Observable) const;

      static void _notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable);

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      RenderStep duplicate(Low::Util::Name p_Name) const;
      static RenderStep duplicate(RenderStep p_Handle,
                                  Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static RenderStep find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == RenderStep::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        RenderStep l_RenderStep = p_Handle.get_id();
        l_RenderStep.destroy();
      }

      Low::Util::Function<bool(RenderStep)>
      get_setup_callback() const;
      void set_setup_callback(
          Low::Util::Function<bool(RenderStep)> p_Value);

      Low::Util::Function<bool(Low::Renderer::RenderStep,
                               Low::Renderer::RenderView)>
      get_prepare_callback() const;
      void set_prepare_callback(
          Low::Util::Function<bool(Low::Renderer::RenderStep,
                                   Low::Renderer::RenderView)>
              p_Value);

      Low::Util::Function<bool(Low::Renderer::RenderStep,
                               Low::Renderer::RenderView)>
      get_teardown_callback() const;
      void set_teardown_callback(
          Low::Util::Function<bool(Low::Renderer::RenderStep,
                                   Low::Renderer::RenderView)>
              p_Value);

      Low::Util::Function<bool(Low::Renderer::RenderStep, float,
                               Low::Renderer::RenderView)>
      get_execute_callback() const;
      void set_execute_callback(
          Low::Util::Function<bool(Low::Renderer::RenderStep, float,
                                   Low::Renderer::RenderView)>
              p_Value);

      Low::Util::Function<bool(Low::Renderer::RenderStep,
                               Low::Math::UVector2 p_NewDimensions,
                               Low::Renderer::RenderView)>
      get_resolution_update_callback() const;
      void set_resolution_update_callback(
          Low::Util::Function<
              bool(Low::Renderer::RenderStep,
                   Low::Math::UVector2 p_NewDimensions,
                   Low::Renderer::RenderView)>
              p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      bool prepare(Low::Renderer::RenderView p_RenderView);
      bool teardown(Low::Renderer::RenderView p_RenderView);
      bool update_resolution(Low::Math::UVector2 &p_NewDimensions,
                             Low::Renderer::RenderView p_RenderView);
      bool execute(float p_DeltaTime,
                   Low::Renderer::RenderView p_RenderView);
      bool setup();

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
