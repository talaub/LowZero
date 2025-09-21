#include "LowRendererRenderpass.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowRendererInterface.h"
#include "LowRendererImage.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t Renderpass::TYPE_ID = 2;
      uint32_t Renderpass::ms_Capacity = 0u;
      uint32_t Renderpass::ms_PageSize = 0u;
      Low::Util::SharedMutex Renderpass::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          Renderpass::ms_PagesLock(Renderpass::ms_PagesMutex,
                                   std::defer_lock);
      Low::Util::List<Renderpass> Renderpass::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          Renderpass::ms_Pages;

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

      Low::Util::Handle Renderpass::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Renderpass Renderpass::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        Renderpass l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Renderpass::TYPE_ID;

        l_PageLock.unlock();

        Low::Util::HandleLock<Renderpass> l_HandleLock(l_Handle);

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Renderpass, renderpass,
                                   Backend::Renderpass))
            Backend::Renderpass();
        ACCESSOR_TYPE_SOA(l_Handle, Renderpass, name,
                          Low::Util::Name) = Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Renderpass::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<Renderpass> l_Lock(get_id());
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

          if (!get_renderpass().swapchainRenderpass) {
            Backend::callbacks().renderpass_cleanup(get_renderpass());
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
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end();) {
          if (it->get_id() == get_id()) {
            it = ms_LivingInstances.erase(it);
          } else {
            it++;
          }
        }
        ms_PagesLock.unlock();
      }

      void Renderpass::initialize()
      {
        LOCK_PAGES_WRITE(l_PagesLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                      N(Renderpass));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Renderpass::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Renderpass);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Renderpass::is_alive;
        l_TypeInfo.destroy = &Renderpass::destroy;
        l_TypeInfo.serialize = &Renderpass::serialize;
        l_TypeInfo.deserialize = &Renderpass::deserialize;
        l_TypeInfo.find_by_index = &Renderpass::_find_by_index;
        l_TypeInfo.notify = &Renderpass::_notify;
        l_TypeInfo.find_by_name = &Renderpass::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Renderpass::_make;
        l_TypeInfo.duplicate_default = &Renderpass::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Renderpass::living_instances);
        l_TypeInfo.get_living_count = &Renderpass::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: renderpass
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(renderpass);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Renderpass::Data, renderpass);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Renderpass l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Renderpass> l_HandleLock(l_Handle);
            l_Handle.get_renderpass();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Renderpass,
                                              renderpass,
                                              Backend::Renderpass);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Renderpass l_Handle = p_Handle.get_id();
            l_Handle.set_renderpass(*(Backend::Renderpass *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Renderpass l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Renderpass> l_HandleLock(l_Handle);
            *((Backend::Renderpass *)p_Data) =
                l_Handle.get_renderpass();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: renderpass
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Renderpass::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Renderpass l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Renderpass> l_HandleLock(l_Handle);
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Renderpass,
                                              name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Renderpass l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Renderpass l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Renderpass> l_HandleLock(l_Handle);
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Function: make
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(make);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType = Renderpass::TYPE_ID;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Name);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::NAME;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Params);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: make
        }
        {
          // Function: get_dimensions
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_dimensions);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_dimensions
        }
        {
          // Function: begin
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(begin);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: begin
        }
        {
          // Function: end
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(end);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: end
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void Renderpass::cleanup()
      {
        Low::Util::List<Renderpass> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle Renderpass::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Renderpass Renderpass::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Renderpass l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Renderpass::TYPE_ID;

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

      Renderpass Renderpass::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Renderpass l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Renderpass::TYPE_ID;

        return l_Handle;
      }

      bool Renderpass::is_alive() const
      {
        if (m_Data.m_Type != Renderpass::TYPE_ID) {
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
        return m_Data.m_Type == Renderpass::TYPE_ID &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Renderpass::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      Renderpass::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      Renderpass Renderpass::find_by_name(Low::Util::Name p_Name)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:FIND_BY_NAME
        // LOW_CODEGEN::END::CUSTOM:FIND_BY_NAME

        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
        return Low::Util::Handle::DEAD;
      }

      Renderpass Renderpass::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        Renderpass l_Handle = make(p_Name);
        l_Handle.set_renderpass(get_renderpass());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      Renderpass Renderpass::duplicate(Renderpass p_Handle,
                                       Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      Renderpass::_duplicate(Low::Util::Handle p_Handle,
                             Low::Util::Name p_Name)
      {
        Renderpass l_Renderpass = p_Handle.get_id();
        return l_Renderpass.duplicate(p_Name);
      }

      void Renderpass::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Renderpass::serialize(Low::Util::Handle p_Handle,
                                 Low::Util::Yaml::Node &p_Node)
      {
        Renderpass l_Renderpass = p_Handle.get_id();
        l_Renderpass.serialize(p_Node);
      }

      Low::Util::Handle
      Renderpass::deserialize(Low::Util::Yaml::Node &p_Node,
                              Low::Util::Handle p_Creator)
      {
        Renderpass l_Handle = Renderpass::make(N(Renderpass));

        if (p_Node["renderpass"]) {
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void Renderpass::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64
      Renderpass::observe(Low::Util::Name p_Observable,
                          Low::Util::Function<void(Low::Util::Handle,
                                                   Low::Util::Name)>
                              p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Renderpass::observe(Low::Util::Name p_Observable,
                              Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Renderpass::notify(Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Renderpass::_notify(Low::Util::Handle p_Observer,
                               Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
      {
        Renderpass l_Renderpass = p_Observer.get_id();
        l_Renderpass.notify(p_Observed, p_Observable);
      }

      Backend::Renderpass &Renderpass::get_renderpass() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Renderpass> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_renderpass

        // LOW_CODEGEN::END::CUSTOM:GETTER_renderpass

        return TYPE_SOA(Renderpass, renderpass, Backend::Renderpass);
      }
      void Renderpass::set_renderpass(Backend::Renderpass &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Renderpass> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_renderpass

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_renderpass

        // Set new value
        TYPE_SOA(Renderpass, renderpass, Backend::Renderpass) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_renderpass

        // LOW_CODEGEN::END::CUSTOM:SETTER_renderpass

        broadcast_observable(N(renderpass));
      }

      Low::Util::Name Renderpass::get_name() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Renderpass> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Renderpass, name, Low::Util::Name);
      }
      void Renderpass::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Renderpass> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(Renderpass, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      Renderpass Renderpass::make(Util::Name p_Name,
                                  RenderpassCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make

        LOW_ASSERT(p_Params.clearColors.size() ==
                       p_Params.renderTargets.size(),
                   "You have to submit the same amount of "
                   "clearcolors as rendertargets");

        Renderpass l_Renderpass = Renderpass::make(p_Name);

        Backend::RenderpassCreateParams l_Params;
        l_Params.context = &p_Params.context.get_context();
        l_Params.clearDepthColor = p_Params.clearDepthColor;
        l_Params.useDepth = p_Params.useDepth;
        if (p_Params.useDepth) {
          l_Params.depthRenderTarget =
              &p_Params.depthRenderTarget.get_image();
        }
        l_Params.dimensions = p_Params.dimensions;
        l_Params.clearTargetColor = p_Params.clearColors.data();
        l_Params.renderTargetCount =
            static_cast<uint8_t>(p_Params.renderTargets.size());

        Util::List<Backend::ImageResource> l_TargetResources;
        for (uint8_t i = 0u; i < l_Params.renderTargetCount; ++i) {
          l_TargetResources.push_back(
              p_Params.renderTargets[i].get_image());
        }
        l_Params.renderTargets = l_TargetResources.data();

        Backend::callbacks().renderpass_create(
            l_Renderpass.get_renderpass(), l_Params);

        return l_Renderpass;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      Math::UVector2 &Renderpass::get_dimensions()
      {
        Low::Util::HandleLock<Renderpass> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_dimensions

        return get_renderpass().dimensions;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_dimensions
      }

      void Renderpass::begin()
      {
        Low::Util::HandleLock<Renderpass> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_begin

        Backend::callbacks().renderpass_begin(get_renderpass());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_begin
      }

      void Renderpass::end()
      {
        Low::Util::HandleLock<Renderpass> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_end

        Backend::callbacks().renderpass_end(get_renderpass());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_end
      }

      uint32_t Renderpass::create_instance(
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

      u32 Renderpass::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Renderpass.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Renderpass::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Renderpass::get_page_for_index(const u32 p_Index,
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

    } // namespace Interface
  } // namespace Renderer
} // namespace Low
