#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#define RENDERSTEP_SOLID_MATERIAL_NAME N(solid_material)
#define RENDERSTEP_LIGHTING_NAME N(phong_lighting)
#define RENDERSTEP_LIGHTCULLING_NAME N(basic_light_culling)
#define RENDERSTEP_SSAO_NAME N(basic_ssao)
#define RENDERSTEP_CAVITIES_NAME N(cavities)
#define RENDERSTEP_UI_NAME N(ui)
#define RENDERSTEP_DEBUG_GEOMETRY_NAME N(debug_geometry)
#define RENDERSTEP_OBJECT_ID_COPY N(object_id_copy)
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    struct RenderView;
    struct RenderStep;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API RenderStep : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
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
          return sizeof(Data);
        }
      };

    public:
      static Low::Util::SharedMutex ms_LivingMutex;
      static Low::Util::UniqueLock<Low::Util::SharedMutex>
          ms_PagesLock;
      static Low::Util::SharedMutex ms_PagesMutex;
      static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

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
        Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static RenderStep *living_instances()
      {
        Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        return ms_LivingInstances.data();
      }

      static RenderStep create_handle_by_index(u32 p_Index);

      static RenderStep find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      u64 observe(Low::Util::Name p_Observable,
                  Low::Util::Handle p_Observer) const;
      u64 observe(Low::Util::Name p_Observable,
                  Low::Util::Function<void(Low::Util::Handle,
                                           Low::Util::Name)>
                      p_Observer) const;
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
        RenderStep l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
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
      static bool get_page_for_index(const u32 p_Index,
                                     u32 &p_PageIndex,
                                     u32 &p_SlotIndex);

    private:
      static u32 ms_Capacity;
      static u32 ms_PageSize;
      static u32 create_instance(
          u32 &p_PageIndex, u32 &p_SlotIndex,
          Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock);
      static u32 create_page();

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
