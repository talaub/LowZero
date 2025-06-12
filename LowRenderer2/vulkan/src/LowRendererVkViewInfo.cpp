#include "LowRendererVkViewInfo.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowRendererVulkan.h"
#include "LowRendererVulkanBuffer.h"
#include "LowMath.h"

#define STAGING_BUFFER_SIZE (8 * LOW_KILOBYTE_I)
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t ViewInfo::TYPE_ID = 55;
      uint32_t ViewInfo::ms_Capacity = 0u;
      uint8_t *ViewInfo::ms_Buffer = 0;
      std::shared_mutex ViewInfo::ms_BufferMutex;
      Low::Util::Instances::Slot *ViewInfo::ms_Slots = 0;
      Low::Util::List<ViewInfo> ViewInfo::ms_LivingInstances =
          Low::Util::List<ViewInfo>();

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
        WRITE_LOCK(l_Lock);
        uint32_t l_Index = create_instance();

        ViewInfo l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = ViewInfo::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, ViewInfo, view_data_buffer,
                                AllocatedBuffer)) AllocatedBuffer();
        new (&ACCESSOR_TYPE_SOA(l_Handle, ViewInfo,
                                view_data_descriptor_set,
                                VkDescriptorSet)) VkDescriptorSet();
        new (&ACCESSOR_TYPE_SOA(l_Handle, ViewInfo,
                                lighting_descriptor_set,
                                VkDescriptorSet)) VkDescriptorSet();
        new (&ACCESSOR_TYPE_SOA(l_Handle, ViewInfo, staging_buffers,
                                Low::Util::List<StagingBuffer>))
            Low::Util::List<StagingBuffer>();
        ACCESSOR_TYPE_SOA(l_Handle, ViewInfo, initialized, bool) =
            false;
        new (&ACCESSOR_TYPE_SOA(l_Handle, ViewInfo,
                                gbuffer_descriptor_set,
                                VkDescriptorSet)) VkDescriptorSet();
        ACCESSOR_TYPE_SOA(l_Handle, ViewInfo, name, Low::Util::Name) =
            Low::Util::Name(0u);
        LOCK_UNLOCK(l_Lock);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        AllocatedBuffer l_Buffer = BufferUtil::create_buffer(
            sizeof(ViewInfoFrameData),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
        l_Handle.set_view_data_buffer(l_Buffer);

        l_Handle.set_view_data_descriptor_set(
            Global::get_global_descriptor_allocator().allocate(
                Global::get_device(),
                Global::get_view_info_descriptor_set_layout()));

        DescriptorUtil::DescriptorWriter l_Writer;
        l_Writer.write_buffer(0,
                              l_Handle.get_view_data_buffer().buffer,
                              sizeof(ViewInfoFrameData), 0,
                              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        l_Writer.update_set(Global::get_device(),
                            l_Handle.get_view_data_descriptor_set());

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

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        BufferUtil::destroy_buffer(get_view_data_buffer());

        for (u32 i = 0u; i < Global::get_frame_overlap(); ++i) {
          BufferUtil::destroy_buffer(get_staging_buffers()[i].buffer);
        }
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        WRITE_LOCK(l_Lock);
        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const ViewInfo *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
            ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
            l_LivingInstanceFound = true;
            break;
          }
        }
      }

      void ViewInfo::initialize()
      {
        WRITE_LOCK(l_Lock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                      N(ViewInfo));

        initialize_buffer(&ms_Buffer, ViewInfoData::get_size(),
                          get_capacity(), &ms_Slots);
        LOCK_UNLOCK(l_Lock);

        LOW_PROFILE_ALLOC(type_buffer_ViewInfo);
        LOW_PROFILE_ALLOC(type_slots_ViewInfo);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(ViewInfo);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &ViewInfo::is_alive;
        l_TypeInfo.destroy = &ViewInfo::destroy;
        l_TypeInfo.serialize = &ViewInfo::serialize;
        l_TypeInfo.deserialize = &ViewInfo::deserialize;
        l_TypeInfo.find_by_index = &ViewInfo::_find_by_index;
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
              offsetof(ViewInfoData, view_data_buffer);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
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
            *((AllocatedBuffer *)p_Data) =
                l_Handle.get_view_data_buffer();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: view_data_buffer
        }
        {
          // Property: view_data_descriptor_set
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(view_data_descriptor_set);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewInfoData, view_data_descriptor_set);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
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
              offsetof(ViewInfoData, lighting_descriptor_set);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
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
              offsetof(ViewInfoData, staging_buffers);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
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
              offsetof(ViewInfoData, initialized);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
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
              offsetof(ViewInfoData, gbuffer_descriptor_set);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
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
            *((VkDescriptorSet *)p_Data) =
                l_Handle.get_gbuffer_descriptor_set();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: gbuffer_descriptor_set
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ViewInfoData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ViewInfo l_Handle = p_Handle.get_id();
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
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void ViewInfo::cleanup()
      {
        Low::Util::List<ViewInfo> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        WRITE_LOCK(l_Lock);
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_ViewInfo);
        LOW_PROFILE_FREE(type_slots_ViewInfo);
        LOCK_UNLOCK(l_Lock);
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
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = ViewInfo::TYPE_ID;

        return l_Handle;
      }

      bool ViewInfo::is_alive() const
      {
        READ_LOCK(l_Lock);
        return m_Data.m_Type == ViewInfo::TYPE_ID &&
               check_alive(ms_Slots, ViewInfo::get_capacity());
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
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
        return 0ull;
      }

      ViewInfo ViewInfo::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        ViewInfo l_Handle = make(p_Name);
        l_Handle.set_view_data_buffer(get_view_data_buffer());
        l_Handle.set_view_data_descriptor_set(
            get_view_data_descriptor_set());
        l_Handle.set_lighting_descriptor_set(
            get_lighting_descriptor_set());
        l_Handle.set_staging_buffers(get_staging_buffers());
        l_Handle.set_initialized(is_initialized());
        l_Handle.set_gbuffer_descriptor_set(
            get_gbuffer_descriptor_set());

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
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      AllocatedBuffer &ViewInfo::get_view_data_buffer() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_view_data_buffer
        // LOW_CODEGEN::END::CUSTOM:GETTER_view_data_buffer

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(ViewInfo, view_data_buffer, AllocatedBuffer);
      }
      void ViewInfo::set_view_data_buffer(AllocatedBuffer &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_view_data_buffer
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_view_data_buffer

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(ViewInfo, view_data_buffer, AllocatedBuffer) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_view_data_buffer
        // LOW_CODEGEN::END::CUSTOM:SETTER_view_data_buffer
      }

      VkDescriptorSet ViewInfo::get_view_data_descriptor_set() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_view_data_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:GETTER_view_data_descriptor_set

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(ViewInfo, view_data_descriptor_set,
                        VkDescriptorSet);
      }
      void
      ViewInfo::set_view_data_descriptor_set(VkDescriptorSet p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_view_data_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_view_data_descriptor_set

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(ViewInfo, view_data_descriptor_set,
                 VkDescriptorSet) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_view_data_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:SETTER_view_data_descriptor_set
      }

      VkDescriptorSet &ViewInfo::get_lighting_descriptor_set() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_lighting_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:GETTER_lighting_descriptor_set

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(ViewInfo, lighting_descriptor_set,
                        VkDescriptorSet);
      }
      void
      ViewInfo::set_lighting_descriptor_set(VkDescriptorSet &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_lighting_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_lighting_descriptor_set

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(ViewInfo, lighting_descriptor_set, VkDescriptorSet) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_lighting_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:SETTER_lighting_descriptor_set
      }

      Low::Util::List<StagingBuffer> &
      ViewInfo::get_staging_buffers() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_staging_buffers
        // LOW_CODEGEN::END::CUSTOM:GETTER_staging_buffers

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(ViewInfo, staging_buffers,
                        Low::Util::List<StagingBuffer>);
      }
      void ViewInfo::set_staging_buffers(
          Low::Util::List<StagingBuffer> &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_staging_buffers
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_staging_buffers

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(ViewInfo, staging_buffers,
                 Low::Util::List<StagingBuffer>) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_staging_buffers
        // LOW_CODEGEN::END::CUSTOM:SETTER_staging_buffers
      }

      bool ViewInfo::is_initialized() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_initialized
        // LOW_CODEGEN::END::CUSTOM:GETTER_initialized

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(ViewInfo, initialized, bool);
      }
      void ViewInfo::set_initialized(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_initialized
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_initialized

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(ViewInfo, initialized, bool) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_initialized
        // LOW_CODEGEN::END::CUSTOM:SETTER_initialized
      }

      VkDescriptorSet &ViewInfo::get_gbuffer_descriptor_set() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gbuffer_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:GETTER_gbuffer_descriptor_set

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(ViewInfo, gbuffer_descriptor_set,
                        VkDescriptorSet);
      }
      void
      ViewInfo::set_gbuffer_descriptor_set(VkDescriptorSet &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gbuffer_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_gbuffer_descriptor_set

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(ViewInfo, gbuffer_descriptor_set, VkDescriptorSet) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gbuffer_descriptor_set
        // LOW_CODEGEN::END::CUSTOM:SETTER_gbuffer_descriptor_set
      }

      Low::Util::Name ViewInfo::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(ViewInfo, name, Low::Util::Name);
      }
      void ViewInfo::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(ViewInfo, name, Low::Util::Name) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      uint32_t ViewInfo::create_instance()
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

      void ViewInfo::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease =
            std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0,
                   "Could not increase capacity");

        uint8_t *l_NewBuffer = (uint8_t *)malloc(
            (l_Capacity + l_CapacityIncrease) * sizeof(ViewInfoData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(
              &l_NewBuffer[offsetof(ViewInfoData, view_data_buffer) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(ViewInfoData, view_data_buffer) *
                         (l_Capacity)],
              l_Capacity * sizeof(AllocatedBuffer));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ViewInfoData,
                                       view_data_descriptor_set) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ViewInfoData,
                                     view_data_descriptor_set) *
                            (l_Capacity)],
                 l_Capacity * sizeof(VkDescriptorSet));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ViewInfoData,
                                       lighting_descriptor_set) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ViewInfoData,
                                     lighting_descriptor_set) *
                            (l_Capacity)],
                 l_Capacity * sizeof(VkDescriptorSet));
        }
        {
          for (auto it = ms_LivingInstances.begin();
               it != ms_LivingInstances.end(); ++it) {
            ViewInfo i_ViewInfo = *it;

            auto *i_ValPtr = new (
                &l_NewBuffer[offsetof(ViewInfoData, staging_buffers) *
                                 (l_Capacity + l_CapacityIncrease) +
                             (it->get_index() *
                              sizeof(
                                  Low::Util::List<StagingBuffer>))])
                Low::Util::List<StagingBuffer>();
            *i_ValPtr = ACCESSOR_TYPE_SOA(
                i_ViewInfo, ViewInfo, staging_buffers,
                Low::Util::List<StagingBuffer>);
          }
        }
        {
          memcpy(&l_NewBuffer[offsetof(ViewInfoData, initialized) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ViewInfoData, initialized) *
                            (l_Capacity)],
                 l_Capacity * sizeof(bool));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ViewInfoData,
                                       gbuffer_descriptor_set) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ViewInfoData,
                                     gbuffer_descriptor_set) *
                            (l_Capacity)],
                 l_Capacity * sizeof(VkDescriptorSet));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(ViewInfoData, name) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(ViewInfoData, name) * (l_Capacity)],
              l_Capacity * sizeof(Low::Util::Name));
        }
        for (uint32_t i = l_Capacity;
             i < l_Capacity + l_CapacityIncrease; ++i) {
          l_NewSlots[i].m_Occupied = false;
          l_NewSlots[i].m_Generation = 0;
        }
        free(ms_Buffer);
        free(ms_Slots);
        ms_Buffer = l_NewBuffer;
        ms_Slots = l_NewSlots;
        ms_Capacity = l_Capacity + l_CapacityIncrease;

        LOW_LOG_DEBUG << "Auto-increased budget for ViewInfo from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease)
                      << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
