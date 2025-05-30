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

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

#define LOW_RENDERER_MATERIAL_DATA_VECTORS 4
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_RENDERER_API ContextData
      {
        Backend::Context context;
        Util::List<Renderpass> renderpasses;
        PipelineResourceSignature global_signature;
        Resource::Buffer frame_info_buffer;
        Resource::Buffer material_data_buffer;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(ContextData);
        }
      };

      struct LOW_RENDERER_API Context : public Low::Util::Handle
      {
      public:
        static std::shared_mutex ms_BufferMutex;
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

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

        static Context find_by_index(uint32_t p_Index);
        static Low::Util::Handle _find_by_index(uint32_t p_Index);

        bool is_alive() const;

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
          READ_LOCK(l_Lock);
          return p_Handle.get_type() == Context::TYPE_ID &&
                 p_Handle.check_alive(ms_Slots, get_capacity());
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

      private:
        static uint32_t ms_Capacity;
        static uint32_t create_instance();
        static void increase_budget();
        void set_global_signature(PipelineResourceSignature p_Value);
        void set_frame_info_buffer(Resource::Buffer p_Value);
        void set_material_data_buffer(Resource::Buffer p_Value);

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
