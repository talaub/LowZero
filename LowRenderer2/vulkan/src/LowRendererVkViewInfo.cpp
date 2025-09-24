#include "LowRendererVkViewInfo.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowRendererVulkan.h"
#include "LowRendererVulkanBuffer.h"
#include "LowMath.h"

#define STAGING_BUFFER_SIZE (8 * LOW_KILOBYTE_I)

#include "LowRendererGlobals.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t ViewInfo::TYPE_ID = 55;
      uint32_t ViewInfo::ms_Capacity = 0u;
      uint32_t ViewInfo::ms_PageSize = 0u;
      Low::Util::SharedMutex ViewInfo::ms_LivingMutex;
      Low::Util::SharedMutex ViewInfo::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          ViewInfo::ms_PagesLock(ViewInfo::ms_PagesMutex,
                                 std::defer_lock);
      Low::Util::List<ViewInfo> ViewInfo::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          ViewInfo::ms_Pages;

      ViewInfo::ViewInfo() : Low::Util::Handle(0ull)
      {
      }
      ViewInfo::ViewInfo(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      ViewInfo::ViewInfo(ViewInfo &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle ViewInfo::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      ViewInfo ViewInfo::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        ViewInfo l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = ViewInfo::TYPE_ID;

        l_PageLock.unlock();

        Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, ViewInfo,
                                   view_data_buffer, AllocatedBuffer))
            AllocatedBuffer();
        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, ViewInfo, directional_light_buffer,
            AllocatedBuffer)) AllocatedBuffer();
        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, ViewInfo, view_data_descriptor_set,
            VkDescriptorSet)) VkDescriptorSet();
        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, ViewInfo, lighting_descriptor_set,
            VkDescriptorSet)) VkDescriptorSet();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, ViewInfo,
                                   staging_buffers,
                                   Low::Util::List<StagingBuffer>))
            Low::Util::List<StagingBuffer>();
        ACCESSOR_TYPE_SOA(l_Handle, ViewInfo, initialized, bool) =
            false;
        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, ViewInfo, gbuffer_descriptor_set,
            VkDescriptorSet)) VkDescriptorSet();
        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, ViewInfo, point_light_cluster_buffer,
            AllocatedBuffer)) AllocatedBuffer();
        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, ViewInfo, point_light_buffer, AllocatedBuffer))
            AllocatedBuffer();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, ViewInfo, light_clusters,
                                   Low::Math::UVector3))
            Low::Math::UVector3();
        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, ViewInfo, ui_drawcommand_buffer,
            AllocatedBuffer)) AllocatedBuffer();
        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, ViewInfo, debug_geometry_buffer,
            AllocatedBuffer)) AllocatedBuffer();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, ViewInfo,
                                   object_id_buffer, AllocatedBuffer))
            AllocatedBuffer();
        ACCESSOR_TYPE_SOA(l_Handle, ViewInfo, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        {
          Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          ms_LivingInstances.push_back(l_Handle);
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        {
          AllocatedBuffer l_Buffer = BufferUtil::create_buffer(
              sizeof(ViewInfoFrameData),
              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
              VMA_MEMORY_USAGE_GPU_ONLY);
          l_Handle.set_view_data_buffer(l_Buffer);
        }

        {
          AllocatedBuffer l_Buffer = BufferUtil::create_buffer(
              sizeof(DirectionalLightInfo),
              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
              VMA_MEMORY_USAGE_GPU_ONLY);
          l_Handle.set_directional_light_buffer(l_Buffer);
        }

        {
          AllocatedBuffer l_Buffer = BufferUtil::create_buffer(
              sizeof(PointLightInfo) * POINTLIGHT_COUNT,
              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
              VMA_MEMORY_USAGE_GPU_ONLY);
          l_Handle.set_point_light_buffer(l_Buffer);
        }

        {
          AllocatedBuffer l_Buffer = BufferUtil::create_buffer(
              sizeof(DebugGeometryUpload) * DEBUG_GEOMETRY_COUNT,
              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
              VMA_MEMORY_USAGE_GPU_ONLY);
          l_Handle.set_debug_geometry_buffer(l_Buffer);
        }

        l_Handle.set_view_data_descriptor_set(
            Global::get_global_descriptor_allocator().allocate(
                Global::get_device(),
                Global::get_view_info_descriptor_set_layout()));

        {
          l_Handle.get_staging_buffers().resize(
              Global::get_frame_overlap());

          for (u32 i = 0u; i < Global::get_frame_overlap(); ++i) {
            l_Handle.get_staging_buffers()[i].buffer =
                BufferUtil::create_buffer(
                    STAGING_BUFFER_SIZE,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU);
            l_Handle.get_staging_buffers()[i].size =
                STAGING_BUFFER_SIZE;
            l_Handle.get_staging_buffers()[i].occupied = 0u;
          }
        }

        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void ViewInfo::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<ViewInfo> l_Lock(get_id());
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          BufferUtil::destroy_buffer(get_view_data_buffer());
          BufferUtil::destroy_buffer(get_directional_light_buffer());
          BufferUtil::destroy_buffer(
              get_point_light_cluster_buffer());
          BufferUtil::destroy_buffer(get_point_light_buffer());
          BufferUtil::destroy_buffer(get_ui_drawcommand_buffer());
          BufferUtil::destroy_buffer(get_debug_geometry_buffer());

          BufferUtil::destroy_buffer(get_object_id_buffer());

          for (u32 i = 0u; i < Global::get_frame_overlap(); ++i) {
            BufferUtil::destroy_buffer(
                get_staging_buffers()[i].buffer);
          }
          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        _LOW_ASSERT(get_page_for_index(get_index(), l_PageIndex,
                                       l_SlotIndex));
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
            l_Page->mutex);
        l_Page->slots[l_SlotIndex].m_Occupied = false;
        l_Page->slots[l_SlotIndex].m_Generation++;

        ms_PagesLock.lock();
        Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end();) {
          if (it->get_id() == get_id()) {
            it = ms_LivingInstances.erase(it);
          } else {
            it++;
          }
        }
        ms_PagesLock.unlock();
        l_LivingLock.unlock();
      }

      void ViewInfo::initialize()
      {
        LOCK_PAGES_WRITE(l_PagesLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                      N(ViewInfo));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, ViewInfo::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(ViewInfo);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &ViewInfo::is_alive;
        l_TypeInfo.destroy = &ViewInfo::destroy;
        l_TypeInfo.serialize = &ViewInfo::serialize;
        l_TypeInfo.deserialize = &ViewInfo::deserialize;
        l_TypeInfo.find_by_index = &ViewInfo::_find_by_index;
        l_TypeInfo.notify = &ViewInfo::_notify;
        l_TypeInfo.find_by_name = &ViewInfo::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &ViewInfo::_make;
        l_TypeInfo.duplicate_default = &ViewInfo::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &ViewInfo::living_instances);
        l_TypeInfo.get_living_count = &ViewInfo::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: view_data_buffer
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(view_data_buffer);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewInfo::Data, view_data_buffer);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            l_Handle.get_view_data_buffer();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ViewInfo,
                                              view_data_buffer,
                                              AllocatedBuffer);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ViewInfo l_Handle = p_Handle.get_id();
            l_Handle.set_view_data_buffer(*(AllocatedBuffer *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            *((AllocatedBuffer *)p_Data) =
                l_Handle.get_view_data_buffer();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: view_data_buffer
        }
        {
          // Property: directional_light_buffer
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(directional_light_buffer);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewInfo::Data, directional_light_buffer);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            l_Handle.get_directional_light_buffer();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, ViewInfo, directional_light_buffer,
                AllocatedBuffer);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ViewInfo l_Handle = p_Handle.get_id();
            l_Handle.set_directional_light_buffer(
                *(AllocatedBuffer *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            *((AllocatedBuffer *)p_Data) =
                l_Handle.get_directional_light_buffer();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: directional_light_buffer
        }
        {
          // Property: view_data_descriptor_set
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(view_data_descriptor_set);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewInfo::Data, view_data_descriptor_set);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            l_Handle.get_view_data_descriptor_set();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, ViewInfo, view_data_descriptor_set,
                VkDescriptorSet);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ViewInfo l_Handle = p_Handle.get_id();
            l_Handle.set_view_data_descriptor_set(
                *(VkDescriptorSet *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            *((VkDescriptorSet *)p_Data) =
                l_Handle.get_view_data_descriptor_set();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: view_data_descriptor_set
        }
        {
          // Property: lighting_descriptor_set
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(lighting_descriptor_set);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewInfo::Data, lighting_descriptor_set);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            l_Handle.get_lighting_descriptor_set();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ViewInfo,
                                              lighting_descriptor_set,
                                              VkDescriptorSet);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ViewInfo l_Handle = p_Handle.get_id();
            l_Handle.set_lighting_descriptor_set(
                *(VkDescriptorSet *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            *((VkDescriptorSet *)p_Data) =
                l_Handle.get_lighting_descriptor_set();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: lighting_descriptor_set
        }
        {
          // Property: staging_buffers
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(staging_buffers);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewInfo::Data, staging_buffers);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            l_Handle.get_staging_buffers();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, ViewInfo, staging_buffers,
                Low::Util::List<StagingBuffer>);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ViewInfo l_Handle = p_Handle.get_id();
            l_Handle.set_staging_buffers(
                *(Low::Util::List<StagingBuffer> *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            *((Low::Util::List<StagingBuffer> *)p_Data) =
                l_Handle.get_staging_buffers();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: staging_buffers
        }
        {
          // Property: initialized
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(initialized);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewInfo::Data, initialized);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            l_Handle.is_initialized();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ViewInfo,
                                              initialized, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ViewInfo l_Handle = p_Handle.get_id();
            l_Handle.set_initialized(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            *((bool *)p_Data) = l_Handle.is_initialized();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: initialized
        }
        {
          // Property: gbuffer_descriptor_set
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(gbuffer_descriptor_set);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewInfo::Data, gbuffer_descriptor_set);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            l_Handle.get_gbuffer_descriptor_set();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ViewInfo,
                                              gbuffer_descriptor_set,
                                              VkDescriptorSet);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ViewInfo l_Handle = p_Handle.get_id();
            l_Handle.set_gbuffer_descriptor_set(
                *(VkDescriptorSet *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            *((VkDescriptorSet *)p_Data) =
                l_Handle.get_gbuffer_descriptor_set();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: gbuffer_descriptor_set
        }
        {
          // Property: point_light_cluster_buffer
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(point_light_cluster_buffer);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewInfo::Data, point_light_cluster_buffer);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            l_Handle.get_point_light_cluster_buffer();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, ViewInfo, point_light_cluster_buffer,
                AllocatedBuffer);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ViewInfo l_Handle = p_Handle.get_id();
            l_Handle.set_point_light_cluster_buffer(
                *(AllocatedBuffer *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            *((AllocatedBuffer *)p_Data) =
                l_Handle.get_point_light_cluster_buffer();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: point_light_cluster_buffer
        }
        {
          // Property: point_light_buffer
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(point_light_buffer);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewInfo::Data, point_light_buffer);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            l_Handle.get_point_light_buffer();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ViewInfo,
                                              point_light_buffer,
                                              AllocatedBuffer);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ViewInfo l_Handle = p_Handle.get_id();
            l_Handle.set_point_light_buffer(
                *(AllocatedBuffer *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            *((AllocatedBuffer *)p_Data) =
                l_Handle.get_point_light_buffer();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: point_light_buffer
        }
        {
          // Property: light_clusters
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(light_clusters);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewInfo::Data, light_clusters);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            l_Handle.get_light_clusters();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ViewInfo,
                                              light_clusters,
                                              Low::Math::UVector3);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ViewInfo l_Handle = p_Handle.get_id();
            l_Handle.set_light_clusters(
                *(Low::Math::UVector3 *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            *((Low::Math::UVector3 *)p_Data) =
                l_Handle.get_light_clusters();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: light_clusters
        }
        {
          // Property: light_cluster_count
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(light_cluster_count);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewInfo::Data, light_cluster_count);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            l_Handle.get_light_cluster_count();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, ViewInfo, light_cluster_count, uint32_t);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ViewInfo l_Handle = p_Handle.get_id();
            l_Handle.set_light_cluster_count(*(uint32_t *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            *((uint32_t *)p_Data) =
                l_Handle.get_light_cluster_count();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: light_cluster_count
        }
        {
          // Property: ui_drawcommand_buffer
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(ui_drawcommand_buffer);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewInfo::Data, ui_drawcommand_buffer);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            l_Handle.get_ui_drawcommand_buffer();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ViewInfo,
                                              ui_drawcommand_buffer,
                                              AllocatedBuffer);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ViewInfo l_Handle = p_Handle.get_id();
            l_Handle.set_ui_drawcommand_buffer(
                *(AllocatedBuffer *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            *((AllocatedBuffer *)p_Data) =
                l_Handle.get_ui_drawcommand_buffer();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: ui_drawcommand_buffer
        }
        {
          // Property: debug_geometry_buffer
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(debug_geometry_buffer);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewInfo::Data, debug_geometry_buffer);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            l_Handle.get_debug_geometry_buffer();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ViewInfo,
                                              debug_geometry_buffer,
                                              AllocatedBuffer);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ViewInfo l_Handle = p_Handle.get_id();
            l_Handle.set_debug_geometry_buffer(
                *(AllocatedBuffer *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            *((AllocatedBuffer *)p_Data) =
                l_Handle.get_debug_geometry_buffer();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: debug_geometry_buffer
        }
        {
          // Property: object_id_buffer
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(object_id_buffer);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewInfo::Data, object_id_buffer);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            l_Handle.get_object_id_buffer();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ViewInfo,
                                              object_id_buffer,
                                              AllocatedBuffer);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ViewInfo l_Handle = p_Handle.get_id();
            l_Handle.set_object_id_buffer(*(AllocatedBuffer *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            *((AllocatedBuffer *)p_Data) =
                l_Handle.get_object_id_buffer();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: object_id_buffer
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ViewInfo::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ViewInfo,
                                              name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ViewInfo l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ViewInfo l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ViewInfo> l_HandleLock(l_Handle);
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Function: get_current_staging_buffer
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_current_staging_buffer);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_current_staging_buffer
        }
        {
          // Function: request_current_staging_buffer_space
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name =
              N(request_current_staging_buffer_space);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_RequestedSize);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_OutOffset);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: request_current_staging_buffer_space
        }
        {
          // Function: write_current_staging_buffer
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(write_current_staging_buffer);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Data);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_DataSize);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Offset);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: write_current_staging_buffer
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void ViewInfo::cleanup()
      {
        Low::Util::List<ViewInfo> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        ms_PagesLock.lock();
        for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
          Low::Util::Instances::Page *i_Page = *it;
          free(i_Page->buffer);
          free(i_Page->slots);
          free(i_Page->lockWords);
          delete i_Page;
          it = ms_Pages.erase(it);
        }

        ms_Capacity = 0;

        ms_PagesLock.unlock();
      }

      Low::Util::Handle ViewInfo::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      ViewInfo ViewInfo::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        ViewInfo l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = ViewInfo::TYPE_ID;

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
          l_Handle.m_Data.m_Generation = 0;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
            l_Page->mutex);
        l_Handle.m_Data.m_Generation =
            l_Page->slots[l_SlotIndex].m_Generation;

        return l_Handle;
      }

      ViewInfo ViewInfo::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        ViewInfo l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = ViewInfo::TYPE_ID;

        return l_Handle;
      }

      bool ViewInfo::is_alive() const
      {
        if (m_Data.m_Type != ViewInfo::TYPE_ID) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
            l_Page->mutex);
        return m_Data.m_Type == ViewInfo::TYPE_ID &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t ViewInfo::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      ViewInfo::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      ViewInfo ViewInfo::find_by_name(Low::Util::Name p_Name)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:FIND_BY_NAME
        // LOW_CODEGEN::END::CUSTOM:FIND_BY_NAME

        Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
        return Low::Util::Handle::DEAD;
      }

      ViewInfo ViewInfo::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        ViewInfo l_Handle = make(p_Name);
        l_Handle.set_view_data_buffer(get_view_data_buffer());
        l_Handle.set_directional_light_buffer(
            get_directional_light_buffer());
        l_Handle.set_view_data_descriptor_set(
            get_view_data_descriptor_set());
        l_Handle.set_lighting_descriptor_set(
            get_lighting_descriptor_set());
        l_Handle.set_staging_buffers(get_staging_buffers());
        l_Handle.set_initialized(is_initialized());
        l_Handle.set_gbuffer_descriptor_set(
            get_gbuffer_descriptor_set());
        l_Handle.set_point_light_cluster_buffer(
            get_point_light_cluster_buffer());
        l_Handle.set_point_light_buffer(get_point_light_buffer());
        l_Handle.set_light_clusters(get_light_clusters());
        l_Handle.set_light_cluster_count(get_light_cluster_count());
        l_Handle.set_ui_drawcommand_buffer(
            get_ui_drawcommand_buffer());
        l_Handle.set_debug_geometry_buffer(
            get_debug_geometry_buffer());
        l_Handle.set_object_id_buffer(get_object_id_buffer());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      ViewInfo ViewInfo::duplicate(ViewInfo p_Handle,
                                   Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      ViewInfo::_duplicate(Low::Util::Handle p_Handle,
                           Low::Util::Name p_Name)
      {
        ViewInfo l_ViewInfo = p_Handle.get_id();
        return l_ViewInfo.duplicate(p_Name);
      }

      void ViewInfo::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["initialized"] = is_initialized();
        p_Node["light_cluster_count"] = get_light_cluster_count();
        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void ViewInfo::serialize(Low::Util::Handle p_Handle,
                               Low::Util::Yaml::Node &p_Node)
      {
        ViewInfo l_ViewInfo = p_Handle.get_id();
        l_ViewInfo.serialize(p_Node);
      }

      Low::Util::Handle
      ViewInfo::deserialize(Low::Util::Yaml::Node &p_Node,
                            Low::Util::Handle p_Creator)
      {
        ViewInfo l_Handle = ViewInfo::make(N(ViewInfo));

        if (p_Node["view_data_buffer"]) {
        }
        if (p_Node["directional_light_buffer"]) {
        }
        if (p_Node["view_data_descriptor_set"]) {
        }
        if (p_Node["lighting_descriptor_set"]) {
        }
        if (p_Node["staging_buffers"]) {
        }
        if (p_Node["initialized"]) {
          l_Handle.set_initialized(p_Node["initialized"].as<bool>());
        }
        if (p_Node["gbuffer_descriptor_set"]) {
        }
        if (p_Node["point_light_cluster_buffer"]) {
        }
        if (p_Node["point_light_buffer"]) {
        }
        if (p_Node["light_clusters"]) {
        }
        if (p_Node["light_cluster_count"]) {
          l_Handle.set_light_cluster_count(
              p_Node["light_cluster_count"].as<uint32_t>());
        }
        if (p_Node["ui_drawcommand_buffer"]) {
        }
        if (p_Node["debug_geometry_buffer"]) {
        }
        if (p_Node["object_id_buffer"]) {
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void ViewInfo::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64
      ViewInfo::observe(Low::Util::Name p_Observable,
                        Low::Util::Function<void(Low::Util::Handle,
                                                 Low::Util::Name)>
                            p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 ViewInfo::observe(Low::Util::Name p_Observable,
                            Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void ViewInfo::notify(Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void ViewInfo::_notify(Low::Util::Handle p_Observer,
                             Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
      {
        ViewInfo l_ViewInfo = p_Observer.get_id();
        l_ViewInfo.notify(p_Observed, p_Observable);
      }

      AllocatedBuffer &ViewInfo::get_view_data_buffer() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_view_data_buffer
        // LOW_CODEGEN::END::CUSTOM:GETTER_view_data_buffer

        return TYPE_SOA(ViewInfo, view_data_buffer, AllocatedBuffer);
      }
      void ViewInfo::set_view_data_buffer(AllocatedBuffer &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_view_data_buffer
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_view_data_buffer

        // Set new value
        TYPE_SOA(ViewInfo, view_data_buffer, AllocatedBuffer) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_view_data_buffer
        // LOW_CODEGEN::END::CUSTOM:SETTER_view_data_buffer

        broadcast_observable(N(view_data_buffer));
      }

      AllocatedBuffer &ViewInfo::get_directional_light_buffer() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_directional_light_buffer
        // LOW_CODEGEN::END::CUSTOM:GETTER_directional_light_buffer

        return TYPE_SOA(ViewInfo, directional_light_buffer,
                        AllocatedBuffer);
      }
      void
      ViewInfo::set_directional_light_buffer(AllocatedBuffer &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_directional_light_buffer
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_directional_light_buffer

        // Set new value
        TYPE_SOA(ViewInfo, directional_light_buffer,
                 AllocatedBuffer) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_directional_light_buffer
        // LOW_CODEGEN::END::CUSTOM:SETTER_directional_light_buffer

        broadcast_observable(N(directional_light_buffer));
      }

      VkDescriptorSet ViewInfo::get_view_data_descriptor_set() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_view_data_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:GETTER_view_data_descriptor_set

        return TYPE_SOA(ViewInfo, view_data_descriptor_set,
                        VkDescriptorSet);
      }
      void
      ViewInfo::set_view_data_descriptor_set(VkDescriptorSet p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_view_data_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_view_data_descriptor_set

        // Set new value
        TYPE_SOA(ViewInfo, view_data_descriptor_set,
                 VkDescriptorSet) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_view_data_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:SETTER_view_data_descriptor_set

        broadcast_observable(N(view_data_descriptor_set));
      }

      VkDescriptorSet &ViewInfo::get_lighting_descriptor_set() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_lighting_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:GETTER_lighting_descriptor_set

        return TYPE_SOA(ViewInfo, lighting_descriptor_set,
                        VkDescriptorSet);
      }
      void
      ViewInfo::set_lighting_descriptor_set(VkDescriptorSet &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_lighting_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_lighting_descriptor_set

        // Set new value
        TYPE_SOA(ViewInfo, lighting_descriptor_set, VkDescriptorSet) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_lighting_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:SETTER_lighting_descriptor_set

        broadcast_observable(N(lighting_descriptor_set));
      }

      Low::Util::List<StagingBuffer> &
      ViewInfo::get_staging_buffers() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_staging_buffers
        // LOW_CODEGEN::END::CUSTOM:GETTER_staging_buffers

        return TYPE_SOA(ViewInfo, staging_buffers,
                        Low::Util::List<StagingBuffer>);
      }
      void ViewInfo::set_staging_buffers(
          Low::Util::List<StagingBuffer> &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_staging_buffers
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_staging_buffers

        // Set new value
        TYPE_SOA(ViewInfo, staging_buffers,
                 Low::Util::List<StagingBuffer>) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_staging_buffers
        // LOW_CODEGEN::END::CUSTOM:SETTER_staging_buffers

        broadcast_observable(N(staging_buffers));
      }

      bool ViewInfo::is_initialized() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_initialized
        // LOW_CODEGEN::END::CUSTOM:GETTER_initialized

        return TYPE_SOA(ViewInfo, initialized, bool);
      }
      void ViewInfo::toggle_initialized()
      {
        set_initialized(!is_initialized());
      }

      void ViewInfo::set_initialized(bool p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_initialized
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_initialized

        // Set new value
        TYPE_SOA(ViewInfo, initialized, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_initialized
        // LOW_CODEGEN::END::CUSTOM:SETTER_initialized

        broadcast_observable(N(initialized));
      }

      VkDescriptorSet &ViewInfo::get_gbuffer_descriptor_set() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gbuffer_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:GETTER_gbuffer_descriptor_set

        return TYPE_SOA(ViewInfo, gbuffer_descriptor_set,
                        VkDescriptorSet);
      }
      void
      ViewInfo::set_gbuffer_descriptor_set(VkDescriptorSet &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gbuffer_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_gbuffer_descriptor_set

        // Set new value
        TYPE_SOA(ViewInfo, gbuffer_descriptor_set, VkDescriptorSet) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gbuffer_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:SETTER_gbuffer_descriptor_set

        broadcast_observable(N(gbuffer_descriptor_set));
      }

      AllocatedBuffer &
      ViewInfo::get_point_light_cluster_buffer() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_point_light_cluster_buffer
        // LOW_CODEGEN::END::CUSTOM:GETTER_point_light_cluster_buffer

        return TYPE_SOA(ViewInfo, point_light_cluster_buffer,
                        AllocatedBuffer);
      }
      void ViewInfo::set_point_light_cluster_buffer(
          AllocatedBuffer &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_point_light_cluster_buffer
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_point_light_cluster_buffer

        // Set new value
        TYPE_SOA(ViewInfo, point_light_cluster_buffer,
                 AllocatedBuffer) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_point_light_cluster_buffer
        // LOW_CODEGEN::END::CUSTOM:SETTER_point_light_cluster_buffer

        broadcast_observable(N(point_light_cluster_buffer));
      }

      AllocatedBuffer &ViewInfo::get_point_light_buffer() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_point_light_buffer
        // LOW_CODEGEN::END::CUSTOM:GETTER_point_light_buffer

        return TYPE_SOA(ViewInfo, point_light_buffer,
                        AllocatedBuffer);
      }
      void ViewInfo::set_point_light_buffer(AllocatedBuffer &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_point_light_buffer
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_point_light_buffer

        // Set new value
        TYPE_SOA(ViewInfo, point_light_buffer, AllocatedBuffer) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_point_light_buffer
        // LOW_CODEGEN::END::CUSTOM:SETTER_point_light_buffer

        broadcast_observable(N(point_light_buffer));
      }

      Low::Math::UVector3 &ViewInfo::get_light_clusters() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_light_clusters
        // LOW_CODEGEN::END::CUSTOM:GETTER_light_clusters

        return TYPE_SOA(ViewInfo, light_clusters,
                        Low::Math::UVector3);
      }
      void ViewInfo::set_light_clusters(Low::Math::UVector3 &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_light_clusters
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_light_clusters

        // Set new value
        TYPE_SOA(ViewInfo, light_clusters, Low::Math::UVector3) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_light_clusters
        // LOW_CODEGEN::END::CUSTOM:SETTER_light_clusters

        broadcast_observable(N(light_clusters));
      }

      uint32_t ViewInfo::get_light_cluster_count() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_light_cluster_count
        // LOW_CODEGEN::END::CUSTOM:GETTER_light_cluster_count

        return TYPE_SOA(ViewInfo, light_cluster_count, uint32_t);
      }
      void ViewInfo::set_light_cluster_count(uint32_t p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_light_cluster_count
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_light_cluster_count

        // Set new value
        TYPE_SOA(ViewInfo, light_cluster_count, uint32_t) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_light_cluster_count
        // LOW_CODEGEN::END::CUSTOM:SETTER_light_cluster_count

        broadcast_observable(N(light_cluster_count));
      }

      AllocatedBuffer &ViewInfo::get_ui_drawcommand_buffer() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_ui_drawcommand_buffer
        // LOW_CODEGEN::END::CUSTOM:GETTER_ui_drawcommand_buffer

        return TYPE_SOA(ViewInfo, ui_drawcommand_buffer,
                        AllocatedBuffer);
      }
      void
      ViewInfo::set_ui_drawcommand_buffer(AllocatedBuffer &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_ui_drawcommand_buffer
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_ui_drawcommand_buffer

        // Set new value
        TYPE_SOA(ViewInfo, ui_drawcommand_buffer, AllocatedBuffer) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_ui_drawcommand_buffer
        // LOW_CODEGEN::END::CUSTOM:SETTER_ui_drawcommand_buffer

        broadcast_observable(N(ui_drawcommand_buffer));
      }

      AllocatedBuffer &ViewInfo::get_debug_geometry_buffer() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_debug_geometry_buffer
        // LOW_CODEGEN::END::CUSTOM:GETTER_debug_geometry_buffer

        return TYPE_SOA(ViewInfo, debug_geometry_buffer,
                        AllocatedBuffer);
      }
      void
      ViewInfo::set_debug_geometry_buffer(AllocatedBuffer &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_debug_geometry_buffer
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_debug_geometry_buffer

        // Set new value
        TYPE_SOA(ViewInfo, debug_geometry_buffer, AllocatedBuffer) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_debug_geometry_buffer
        // LOW_CODEGEN::END::CUSTOM:SETTER_debug_geometry_buffer

        broadcast_observable(N(debug_geometry_buffer));
      }

      AllocatedBuffer &ViewInfo::get_object_id_buffer() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_object_id_buffer
        // LOW_CODEGEN::END::CUSTOM:GETTER_object_id_buffer

        return TYPE_SOA(ViewInfo, object_id_buffer, AllocatedBuffer);
      }
      void ViewInfo::set_object_id_buffer(AllocatedBuffer &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_object_id_buffer
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_object_id_buffer

        // Set new value
        TYPE_SOA(ViewInfo, object_id_buffer, AllocatedBuffer) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_object_id_buffer
        // LOW_CODEGEN::END::CUSTOM:SETTER_object_id_buffer

        broadcast_observable(N(object_id_buffer));
      }

      Low::Util::Name ViewInfo::get_name() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(ViewInfo, name, Low::Util::Name);
      }
      void ViewInfo::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(ViewInfo, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      StagingBuffer &ViewInfo::get_current_staging_buffer()
      {
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_current_staging_buffer
        return get_staging_buffers()
            [Global::get_current_frame_index()];
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_current_staging_buffer
      }

      size_t ViewInfo::request_current_staging_buffer_space(
          const size_t p_RequestedSize, size_t *p_OutOffset)
      {
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_request_current_staging_buffer_space
        StagingBuffer &l_StagingBuffer = get_current_staging_buffer();

        return l_StagingBuffer.request_space(p_RequestedSize,
                                             p_OutOffset);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_request_current_staging_buffer_space
      }

      bool
      ViewInfo::write_current_staging_buffer(void *p_Data,
                                             const size_t p_DataSize,
                                             const size_t p_Offset)
      {
        Low::Util::HandleLock<ViewInfo> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_write_current_staging_buffer
        StagingBuffer &l_StagingBuffer = get_current_staging_buffer();

        return l_StagingBuffer.write(p_Data, p_DataSize, p_Offset);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_write_current_staging_buffer
      }

      uint32_t ViewInfo::create_instance(
          u32 &p_PageIndex, u32 &p_SlotIndex,
          Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock)
      {
        LOCK_PAGES_WRITE(l_PagesLock);
        u32 l_Index = 0;
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        bool l_FoundIndex = false;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;

        for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
             ++l_PageIndex) {
          Low::Util::UniqueLock<Low::Util::Mutex> i_PageLock(
              ms_Pages[l_PageIndex]->mutex);
          for (l_SlotIndex = 0;
               l_SlotIndex < ms_Pages[l_PageIndex]->size;
               ++l_SlotIndex) {
            if (!ms_Pages[l_PageIndex]
                     ->slots[l_SlotIndex]
                     .m_Occupied) {
              l_FoundIndex = true;
              l_PageLock = std::move(i_PageLock);
              break;
            }
            l_Index++;
          }
          if (l_FoundIndex) {
            break;
          }
        }
        if (!l_FoundIndex) {
          l_SlotIndex = 0;
          l_PageIndex = create_page();
          Low::Util::UniqueLock<Low::Util::Mutex> l_NewLock(
              ms_Pages[l_PageIndex]->mutex);
          l_PageLock = std::move(l_NewLock);
        }
        ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
        p_PageIndex = l_PageIndex;
        p_SlotIndex = l_SlotIndex;
        p_PageLock = std::move(l_PageLock);
        LOCK_UNLOCK(l_PagesLock);
        return l_Index;
      }

      u32 ViewInfo::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for ViewInfo.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, ViewInfo::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool ViewInfo::get_page_for_index(const u32 p_Index,
                                        u32 &p_PageIndex,
                                        u32 &p_SlotIndex)
      {
        if (p_Index >= get_capacity()) {
          p_PageIndex = LOW_UINT32_MAX;
          p_SlotIndex = LOW_UINT32_MAX;
          return false;
        }
        p_PageIndex = p_Index / ms_PageSize;
        if (p_PageIndex > (ms_Pages.size() - 1)) {
          return false;
        }
        p_SlotIndex = p_Index - (ms_PageSize * p_PageIndex);
        return true;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Vulkan
  } // namespace Renderer
} // namespace Low
