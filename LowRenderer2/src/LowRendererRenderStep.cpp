#include "LowRendererRenderStep.h"

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
#include "LowRendererRenderView.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t RenderStep::TYPE_ID = 64;
    uint32_t RenderStep::ms_Capacity = 0u;
    uint32_t RenderStep::ms_PageSize = 0u;
    Low::Util::SharedMutex RenderStep::ms_LivingMutex;
    Low::Util::SharedMutex RenderStep::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        RenderStep::ms_PagesLock(RenderStep::ms_PagesMutex,
                                 std::defer_lock);
    Low::Util::List<RenderStep> RenderStep::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        RenderStep::ms_Pages;

    RenderStep::RenderStep() : Low::Util::Handle(0ull)
    {
    }
    RenderStep::RenderStep(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    RenderStep::RenderStep(RenderStep &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle RenderStep::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    RenderStep RenderStep::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      RenderStep l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = RenderStep::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<RenderStep> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderStep, setup_callback,
          Low::Util::Function<bool(RenderStep)>))
          Low::Util::Function<bool(RenderStep)>();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderStep, prepare_callback,
          SINGLE_ARG(
              Low::Util::Function<bool(Low::Renderer::RenderStep,
                                       Low::Renderer::RenderView)>)))
          Low::Util::Function<bool(Low::Renderer::RenderStep,
                                   Low::Renderer::RenderView)>();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderStep, teardown_callback,
          SINGLE_ARG(
              Low::Util::Function<bool(Low::Renderer::RenderStep,
                                       Low::Renderer::RenderView)>)))
          Low::Util::Function<bool(Low::Renderer::RenderStep,
                                   Low::Renderer::RenderView)>();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderStep, execute_callback,
          SINGLE_ARG(Low::Util::Function<bool(
                         Low::Renderer::RenderStep, float,
                         Low::Renderer::RenderView)>)))
          Low::Util::Function<bool(Low::Renderer::RenderStep, float,
                                   Low::Renderer::RenderView)>();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderStep, resolution_update_callback,
          SINGLE_ARG(Low::Util::Function<bool(
                         Low::Renderer::RenderStep,
                         Low::Math::UVector2 p_NewDimensions,
                         Low::Renderer::RenderView)>)))
          Low::Util::Function<bool(
              Low::Renderer::RenderStep,
              Low::Math::UVector2 p_NewDimensions,
              Low::Renderer::RenderView)>();
      ACCESSOR_TYPE_SOA(l_Handle, RenderStep, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      {
        Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        ms_LivingInstances.push_back(l_Handle);
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_setup_callback(
          [&](RenderStep p_RenderStep) -> bool { return true; });
      l_Handle.set_prepare_callback(
          [&](RenderStep p_RenderStep,
              RenderView p_RenderView) -> bool { return true; });
      l_Handle.set_teardown_callback(
          [&](RenderStep p_RenderStep,
              RenderView p_RenderView) -> bool { return true; });
      l_Handle.set_resolution_update_callback(
          [&](RenderStep p_RenderStep,
              Low::Math::UVector2 p_NewDimensions,
              RenderView p_RenderView) -> bool { return true; });
      l_Handle.set_execute_callback(
          [&](RenderStep p_RenderStep, float p_Delta,
              RenderView p_RenderView) -> bool { return true; });
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void RenderStep::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<RenderStep> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      _LOW_ASSERT(
          get_page_for_index(get_index(), l_PageIndex, l_SlotIndex));
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

    void RenderStep::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(RenderStep));

      ms_PageSize = ms_Capacity;
      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, RenderStep::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(RenderStep);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &RenderStep::is_alive;
      l_TypeInfo.destroy = &RenderStep::destroy;
      l_TypeInfo.serialize = &RenderStep::serialize;
      l_TypeInfo.deserialize = &RenderStep::deserialize;
      l_TypeInfo.find_by_index = &RenderStep::_find_by_index;
      l_TypeInfo.notify = &RenderStep::_notify;
      l_TypeInfo.find_by_name = &RenderStep::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &RenderStep::_make;
      l_TypeInfo.duplicate_default = &RenderStep::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &RenderStep::living_instances);
      l_TypeInfo.get_living_count = &RenderStep::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: setup_callback
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(setup_callback);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderStep::Data, setup_callback);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderStep l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderStep> l_HandleLock(l_Handle);
          l_Handle.get_setup_callback();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderStep, setup_callback,
              Low::Util::Function<bool(RenderStep)>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderStep l_Handle = p_Handle.get_id();
          l_Handle.set_setup_callback(
              *(Low::Util::Function<bool(RenderStep)> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderStep l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderStep> l_HandleLock(l_Handle);
          *((Low::Util::Function<bool(RenderStep)> *)p_Data) =
              l_Handle.get_setup_callback();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: setup_callback
      }
      {
        // Property: prepare_callback
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(prepare_callback);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderStep::Data, prepare_callback);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderStep l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderStep> l_HandleLock(l_Handle);
          l_Handle.get_prepare_callback();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderStep, prepare_callback,
              SINGLE_ARG(Low::Util::Function<bool(
                             Low::Renderer::RenderStep,
                             Low::Renderer::RenderView)>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderStep l_Handle = p_Handle.get_id();
          l_Handle.set_prepare_callback(*(
              Low::Util::Function<bool(Low::Renderer::RenderStep,
                                       Low::Renderer::RenderView)> *)
                                            p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderStep l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderStep> l_HandleLock(l_Handle);
          *((Low::Util::Function<bool(Low::Renderer::RenderStep,
                                      Low::Renderer::RenderView)> *)
                p_Data) = l_Handle.get_prepare_callback();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: prepare_callback
      }
      {
        // Property: teardown_callback
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(teardown_callback);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderStep::Data, teardown_callback);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderStep l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderStep> l_HandleLock(l_Handle);
          l_Handle.get_teardown_callback();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderStep, teardown_callback,
              SINGLE_ARG(Low::Util::Function<bool(
                             Low::Renderer::RenderStep,
                             Low::Renderer::RenderView)>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderStep l_Handle = p_Handle.get_id();
          l_Handle.set_teardown_callback(*(
              Low::Util::Function<bool(Low::Renderer::RenderStep,
                                       Low::Renderer::RenderView)> *)
                                             p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderStep l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderStep> l_HandleLock(l_Handle);
          *((Low::Util::Function<bool(Low::Renderer::RenderStep,
                                      Low::Renderer::RenderView)> *)
                p_Data) = l_Handle.get_teardown_callback();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: teardown_callback
      }
      {
        // Property: execute_callback
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(execute_callback);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderStep::Data, execute_callback);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderStep l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderStep> l_HandleLock(l_Handle);
          l_Handle.get_execute_callback();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderStep, execute_callback,
              SINGLE_ARG(Low::Util::Function<bool(
                             Low::Renderer::RenderStep, float,
                             Low::Renderer::RenderView)>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderStep l_Handle = p_Handle.get_id();
          l_Handle.set_execute_callback(
              *(Low::Util::Function<bool(
                    Low::Renderer::RenderStep, float,
                    Low::Renderer::RenderView)> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderStep l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderStep> l_HandleLock(l_Handle);
          *((Low::Util::Function<bool(
                 Low::Renderer::RenderStep, float,
                 Low::Renderer::RenderView)> *)p_Data) =
              l_Handle.get_execute_callback();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: execute_callback
      }
      {
        // Property: resolution_update_callback
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resolution_update_callback);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderStep::Data, resolution_update_callback);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderStep l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderStep> l_HandleLock(l_Handle);
          l_Handle.get_resolution_update_callback();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderStep, resolution_update_callback,
              SINGLE_ARG(Low::Util::Function<bool(
                             Low::Renderer::RenderStep,
                             Low::Math::UVector2 p_NewDimensions,
                             Low::Renderer::RenderView)>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderStep l_Handle = p_Handle.get_id();
          l_Handle.set_resolution_update_callback(
              *(Low::Util::Function<bool(
                    Low::Renderer::RenderStep,
                    Low::Math::UVector2 p_NewDimensions,
                    Low::Renderer::RenderView)> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderStep l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderStep> l_HandleLock(l_Handle);
          *((Low::Util::Function<bool(
                 Low::Renderer::RenderStep,
                 Low::Math::UVector2 p_NewDimensions,
                 Low::Renderer::RenderView)> *)p_Data) =
              l_Handle.get_resolution_update_callback();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: resolution_update_callback
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderStep::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderStep l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderStep> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderStep,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderStep l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderStep l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderStep> l_HandleLock(l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: prepare
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(prepare);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderView);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::RenderView::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: prepare
      }
      {
        // Function: teardown
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(teardown);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderView);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::RenderView::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: teardown
      }
      {
        // Function: update_resolution
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(update_resolution);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_NewDimensions);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderView);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::RenderView::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: update_resolution
      }
      {
        // Function: execute
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(execute);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_DeltaTime);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderView);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::RenderView::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: execute
      }
      {
        // Function: setup
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(setup);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: setup
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void RenderStep::cleanup()
    {
      Low::Util::List<RenderStep> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle RenderStep::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    RenderStep RenderStep::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      RenderStep l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = RenderStep::TYPE_ID;

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

    RenderStep RenderStep::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      RenderStep l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = RenderStep::TYPE_ID;

      return l_Handle;
    }

    bool RenderStep::is_alive() const
    {
      if (m_Data.m_Type != RenderStep::TYPE_ID) {
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
      return m_Data.m_Type == RenderStep::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t RenderStep::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    RenderStep::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    RenderStep RenderStep::find_by_name(Low::Util::Name p_Name)
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

    RenderStep RenderStep::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      RenderStep l_Handle = make(p_Name);
      l_Handle.set_setup_callback(get_setup_callback());
      l_Handle.set_prepare_callback(get_prepare_callback());
      l_Handle.set_teardown_callback(get_teardown_callback());
      l_Handle.set_execute_callback(get_execute_callback());
      l_Handle.set_resolution_update_callback(
          get_resolution_update_callback());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    RenderStep RenderStep::duplicate(RenderStep p_Handle,
                                     Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    RenderStep::_duplicate(Low::Util::Handle p_Handle,
                           Low::Util::Name p_Name)
    {
      RenderStep l_RenderStep = p_Handle.get_id();
      return l_RenderStep.duplicate(p_Name);
    }

    void RenderStep::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void RenderStep::serialize(Low::Util::Handle p_Handle,
                               Low::Util::Yaml::Node &p_Node)
    {
      RenderStep l_RenderStep = p_Handle.get_id();
      l_RenderStep.serialize(p_Node);
    }

    Low::Util::Handle
    RenderStep::deserialize(Low::Util::Yaml::Node &p_Node,
                            Low::Util::Handle p_Creator)
    {
      RenderStep l_Handle = RenderStep::make(N(RenderStep));

      if (p_Node["setup_callback"]) {
      }
      if (p_Node["prepare_callback"]) {
      }
      if (p_Node["teardown_callback"]) {
      }
      if (p_Node["execute_callback"]) {
      }
      if (p_Node["resolution_update_callback"]) {
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void RenderStep::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 RenderStep::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 RenderStep::observe(Low::Util::Name p_Observable,
                            Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void RenderStep::notify(Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void RenderStep::_notify(Low::Util::Handle p_Observer,
                             Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
    {
      RenderStep l_RenderStep = p_Observer.get_id();
      l_RenderStep.notify(p_Observed, p_Observable);
    }

    Low::Util::Function<bool(RenderStep)>
    RenderStep::get_setup_callback() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_setup_callback
      // LOW_CODEGEN::END::CUSTOM:GETTER_setup_callback

      return TYPE_SOA(RenderStep, setup_callback,
                      Low::Util::Function<bool(RenderStep)>);
    }
    void RenderStep::set_setup_callback(
        Low::Util::Function<bool(RenderStep)> p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_setup_callback
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_setup_callback

      // Set new value
      TYPE_SOA(RenderStep, setup_callback,
               Low::Util::Function<bool(RenderStep)>) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_setup_callback
      // LOW_CODEGEN::END::CUSTOM:SETTER_setup_callback

      broadcast_observable(N(setup_callback));
    }

    Low::Util::Function<bool(Low::Renderer::RenderStep,
                             Low::Renderer::RenderView)>
    RenderStep::get_prepare_callback() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_prepare_callback
      // LOW_CODEGEN::END::CUSTOM:GETTER_prepare_callback

      return TYPE_SOA(
          RenderStep, prepare_callback,
          SINGLE_ARG(
              Low::Util::Function<bool(Low::Renderer::RenderStep,
                                       Low::Renderer::RenderView)>));
    }
    void RenderStep::set_prepare_callback(
        Low::Util::Function<bool(Low::Renderer::RenderStep,
                                 Low::Renderer::RenderView)>
            p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_prepare_callback
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_prepare_callback

      // Set new value
      TYPE_SOA(
          RenderStep, prepare_callback,
          SINGLE_ARG(
              Low::Util::Function<bool(Low::Renderer::RenderStep,
                                       Low::Renderer::RenderView)>)) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_prepare_callback
      // LOW_CODEGEN::END::CUSTOM:SETTER_prepare_callback

      broadcast_observable(N(prepare_callback));
    }

    Low::Util::Function<bool(Low::Renderer::RenderStep,
                             Low::Renderer::RenderView)>
    RenderStep::get_teardown_callback() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_teardown_callback
      // LOW_CODEGEN::END::CUSTOM:GETTER_teardown_callback

      return TYPE_SOA(
          RenderStep, teardown_callback,
          SINGLE_ARG(
              Low::Util::Function<bool(Low::Renderer::RenderStep,
                                       Low::Renderer::RenderView)>));
    }
    void RenderStep::set_teardown_callback(
        Low::Util::Function<bool(Low::Renderer::RenderStep,
                                 Low::Renderer::RenderView)>
            p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_teardown_callback
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_teardown_callback

      // Set new value
      TYPE_SOA(
          RenderStep, teardown_callback,
          SINGLE_ARG(
              Low::Util::Function<bool(Low::Renderer::RenderStep,
                                       Low::Renderer::RenderView)>)) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_teardown_callback
      // LOW_CODEGEN::END::CUSTOM:SETTER_teardown_callback

      broadcast_observable(N(teardown_callback));
    }

    Low::Util::Function<bool(Low::Renderer::RenderStep, float,
                             Low::Renderer::RenderView)>
    RenderStep::get_execute_callback() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_execute_callback
      // LOW_CODEGEN::END::CUSTOM:GETTER_execute_callback

      return TYPE_SOA(RenderStep, execute_callback,
                      SINGLE_ARG(Low::Util::Function<bool(
                                     Low::Renderer::RenderStep, float,
                                     Low::Renderer::RenderView)>));
    }
    void RenderStep::set_execute_callback(
        Low::Util::Function<bool(Low::Renderer::RenderStep, float,
                                 Low::Renderer::RenderView)>
            p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_execute_callback
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_execute_callback

      // Set new value
      TYPE_SOA(RenderStep, execute_callback,
               SINGLE_ARG(Low::Util::Function<bool(
                              Low::Renderer::RenderStep, float,
                              Low::Renderer::RenderView)>)) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_execute_callback
      // LOW_CODEGEN::END::CUSTOM:SETTER_execute_callback

      broadcast_observable(N(execute_callback));
    }

    Low::Util::Function<bool(Low::Renderer::RenderStep,
                             Low::Math::UVector2 p_NewDimensions,
                             Low::Renderer::RenderView)>
    RenderStep::get_resolution_update_callback() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resolution_update_callback
      // LOW_CODEGEN::END::CUSTOM:GETTER_resolution_update_callback

      return TYPE_SOA(
          RenderStep, resolution_update_callback,
          SINGLE_ARG(Low::Util::Function<bool(
                         Low::Renderer::RenderStep,
                         Low::Math::UVector2 p_NewDimensions,
                         Low::Renderer::RenderView)>));
    }
    void RenderStep::set_resolution_update_callback(
        Low::Util::Function<bool(Low::Renderer::RenderStep,
                                 Low::Math::UVector2 p_NewDimensions,
                                 Low::Renderer::RenderView)>
            p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_resolution_update_callback
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_resolution_update_callback

      // Set new value
      TYPE_SOA(RenderStep, resolution_update_callback,
               SINGLE_ARG(Low::Util::Function<bool(
                              Low::Renderer::RenderStep,
                              Low::Math::UVector2 p_NewDimensions,
                              Low::Renderer::RenderView)>)) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_resolution_update_callback
      // LOW_CODEGEN::END::CUSTOM:SETTER_resolution_update_callback

      broadcast_observable(N(resolution_update_callback));
    }

    Low::Util::Name RenderStep::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(RenderStep, name, Low::Util::Name);
    }
    void RenderStep::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(RenderStep, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    bool RenderStep::prepare(Low::Renderer::RenderView p_RenderView)
    {
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_prepare
      return get_prepare_callback()(get_id(), p_RenderView);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_prepare
    }

    bool RenderStep::teardown(Low::Renderer::RenderView p_RenderView)
    {
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_teardown
      return get_teardown_callback()(get_id(), p_RenderView);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_teardown
    }

    bool RenderStep::update_resolution(
        Low::Math::UVector2 &p_NewDimensions,
        Low::Renderer::RenderView p_RenderView)
    {
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_update_resolution
      return get_resolution_update_callback()(
          get_id(), p_NewDimensions, p_RenderView);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_update_resolution
    }

    bool RenderStep::execute(float p_DeltaTime,
                             Low::Renderer::RenderView p_RenderView)
    {
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_execute
      return get_execute_callback()(get_id(), p_DeltaTime,
                                    p_RenderView);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_execute
    }

    bool RenderStep::setup()
    {
      Low::Util::HandleLock<RenderStep> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_setup
      return get_setup_callback()(get_id());
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_setup
    }

    uint32_t RenderStep::create_instance(
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
          if (!ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied) {
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
      LOW_ASSERT(l_FoundIndex, "Budget blown for type RenderStep");
      ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
      p_PageIndex = l_PageIndex;
      p_SlotIndex = l_SlotIndex;
      p_PageLock = std::move(l_PageLock);
      LOCK_UNLOCK(l_PagesLock);
      return l_Index;
    }

    u32 RenderStep::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for RenderStep.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, RenderStep::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool RenderStep::get_page_for_index(const u32 p_Index,
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

  } // namespace Renderer
} // namespace Low
