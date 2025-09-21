#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include <imgui.h>
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API EditorImageGpu : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        bool imgui_texture_initialized;
        uint64_t data_handle;
        uint64_t editor_image_handle;
        ImTextureID imgui_texture_id;
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

      static Low::Util::List<EditorImageGpu> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      EditorImageGpu();
      EditorImageGpu(uint64_t p_Id);
      EditorImageGpu(EditorImageGpu &p_Copy);

      static EditorImageGpu make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit EditorImageGpu(const EditorImageGpu &p_Copy)
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
      static EditorImageGpu *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static EditorImageGpu create_handle_by_index(u32 p_Index);

      static EditorImageGpu find_by_index(uint32_t p_Index);
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

      EditorImageGpu duplicate(Low::Util::Name p_Name) const;
      static EditorImageGpu duplicate(EditorImageGpu p_Handle,
                                      Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static EditorImageGpu find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        EditorImageGpu l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        EditorImageGpu l_EditorImageGpu = p_Handle.get_id();
        l_EditorImageGpu.destroy();
      }

      bool is_imgui_texture_initialized() const;
      void set_imgui_texture_initialized(bool p_Value);
      void toggle_imgui_texture_initialized();

      uint64_t get_data_handle() const;
      void set_data_handle(uint64_t p_Value);

      uint64_t get_editor_image_handle() const;
      void set_editor_image_handle(uint64_t p_Value);

      ImTextureID get_imgui_texture_id() const;
      void set_imgui_texture_id(ImTextureID p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

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
    public:
      static Low::Util::Set<Low::Renderer::EditorImageGpu> ms_Dirty;
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
