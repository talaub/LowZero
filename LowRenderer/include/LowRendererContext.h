#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererBackend.h"
#include "LowRendererRenderpass.h"
#include "LowRendererPipelineResourceSignature.h"
#include "LowRendererBuffer.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

#define LOW_RENDERER_MATERIAL_DATA_VECTORS 4
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_RENDERER_API Context : public Low::Util::Handle
      {
      public:
        struct Data
        {
        public:
          Backend::Context context;
          Util::List<Renderpass> renderpasses;
          PipelineResourceSignature global_signature;
          Resource::Buffer frame_info_buffer;
          Resource::Buffer material_data_buffer;
          Low::Util::Name name;

          static size_t get_size()
          {
            return sizeof(Data);
          }
        };

      public:
        static Low::Util::UniqueLock<Low::Util::SharedMutex>
            ms_PagesLock;
        static Low::Util::SharedMutex ms_PagesMutex;
        static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

        static Low::Util::List<Context> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        Context();
        Context(uint64_t p_Id);
        Context(Context &p_Copy);

      private:
        static Context make(Low::Util::Name p_Name);
        static Low::Util::Handle _make(Low::Util::Name p_Name);

      public:
        explicit Context(const Context &p_Copy)
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
        static Context *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static Context create_handle_by_index(u32 p_Index);

        static Context find_by_index(uint32_t p_Index);
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

        Context duplicate(Low::Util::Name p_Name) const;
        static Context duplicate(Context p_Handle,
                                 Low::Util::Name p_Name);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Name p_Name);

        static Context find_by_name(Low::Util::Name p_Name);
        static Low::Util::Handle
        _find_by_name(Low::Util::Name p_Name);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Yaml::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          Context l_Handle = p_Handle.get_id();
          return l_Handle.is_alive();
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          Context l_Context = p_Handle.get_id();
          l_Context.destroy();
        }

        Backend::Context &get_context() const;

        Util::List<Renderpass> &get_renderpasses() const;

        PipelineResourceSignature get_global_signature() const;

        Resource::Buffer get_frame_info_buffer() const;

        Resource::Buffer get_material_data_buffer() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static Context make(Util::Name p_Name, Window *p_Window,
                            uint8_t p_FramesInFlight,
                            bool p_ValidationEnabled);
        uint8_t get_frames_in_flight();
        uint8_t get_image_count();
        uint8_t get_current_frame_index();
        uint8_t get_current_image_index();
        Renderpass get_current_renderpass();
        Math::UVector2 &get_dimensions();
        uint8_t get_image_format();
        Window &get_window();
        uint8_t get_state();
        bool is_debug_enabled();
        void wait_idle();
        uint8_t prepare_frame();
        void render_frame();
        void begin_imgui_frame();
        void render_imgui();
        void update_dimensions();
        void clear_committed_resource_signatures();
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
        void set_global_signature(PipelineResourceSignature p_Value);
        void set_frame_info_buffer(Resource::Buffer p_Value);
        void set_material_data_buffer(Resource::Buffer p_Value);

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Interface
  } // namespace Renderer
} // namespace Low
