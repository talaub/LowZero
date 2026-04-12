#include "LowCoreScriptAsset.h"

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
#include "LowUtilAssetManager.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Scripting {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 Asset::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          Asset::IDENTIFIER(LOW_NAME(1181529166),
                            LOW_NAME(3278796120));
      uint32_t Asset::ms_Capacity = 0u;
      uint32_t Asset::ms_PageSize = 0u;
      Low::Util::SharedMutex Asset::ms_LivingMutex;
      Low::Util::SharedMutex Asset::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          Asset::ms_PagesLock(Asset::ms_PagesMutex, std::defer_lock);
      Low::Util::List<Asset> Asset::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *> Asset::ms_Pages;

      Low::Util::Handle Asset::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Asset Asset::make(Low::Util::Name p_Name)
      {
        return make(p_Name, 0ull);
      }

      Asset Asset::make(Low::Util::Name p_Name,
                        Low::Util::UniqueId p_UniqueId)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        Asset l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Asset::ms_TypeId;

        l_PageLock.unlock();

        Low::Util::HandleLock<Asset> l_HandleLock(l_Handle);

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Asset, module,
                                   Low::Core::Scripting::Module))
            Low::Core::Scripting::Module();
        ACCESSOR_TYPE_SOA(l_Handle, Asset, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        {
          Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          ms_LivingInstances.push_back(l_Handle);
        }

        if (p_UniqueId > 0ull) {
          l_Handle.set_unique_id(p_UniqueId);
        } else {
          l_Handle.set_unique_id(
              Low::Util::generate_unique_id(l_Handle.get_id()));
        }
        Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                      l_Handle.get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Asset::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<Asset> l_Lock(get_id());
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

        Low::Util::remove_unique_id(get_unique_id());

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

      void Asset::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(N(LowCore),
                                                          N(Asset));

        LOCK_PAGES_WRITE(l_PagesLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(Asset));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Asset::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Asset);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Asset::is_alive;
        l_TypeInfo.destroy = &Asset::destroy;
        l_TypeInfo.serialize = &Asset::serialize;
        l_TypeInfo.deserialize = &Asset::deserialize;
        l_TypeInfo.find_by_index = &Asset::_find_by_index;
        l_TypeInfo.notify = &Asset::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &Asset::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Asset::_make;
        l_TypeInfo.duplicate_default = &Asset::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Asset::living_instances);
        l_TypeInfo.get_living_count = &Asset::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: module
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(module);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Asset::Data, module);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Low::Core::Scripting::Module::type_id();
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Asset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Asset> l_HandleLock(l_Handle);
            l_Handle.get_module();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Asset, module,
                Low::Core::Scripting::Module);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Asset l_Handle = p_Handle.get_id();
            l_Handle.set_module(
                *(Low::Core::Scripting::Module *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Asset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Asset> l_HandleLock(l_Handle);
            *((Low::Core::Scripting::Module *)p_Data) =
                l_Handle.get_module();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: module
        }
        {
          // Property: source_path
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(source_path);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Asset::Data, source_path);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Asset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Asset> l_HandleLock(l_Handle);
            l_Handle.get_source_path();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Asset, source_path, Low::Util::String);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Asset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Asset> l_HandleLock(l_Handle);
            *((Low::Util::String *)p_Data) =
                l_Handle.get_source_path();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: source_path
        }
        {
          // Property: unique_id
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Asset::Data, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Asset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Asset> l_HandleLock(l_Handle);
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Asset, unique_id, Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Asset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Asset> l_HandleLock(l_Handle);
            *((Low::Util::UniqueId *)p_Data) =
                l_Handle.get_unique_id();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: unique_id
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Asset::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Asset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Asset> l_HandleLock(l_Handle);
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Asset, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Asset l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Asset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Asset> l_HandleLock(l_Handle);
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        Util::AssetManager::TypeRegistratorBuilder l_Builder(
            N(Script), IDENTIFIER);
        l_Builder.initialize_on_startup(true);
        l_Builder.auto_initialize(true).add_asset_suffix(
            ".script.yaml");
        l_Builder.add_initialize_directory(
            Util::get_project().assetCachePath);
        l_Builder.initializer(
            [](const Util::String p_Path) -> Util::Handle {
              const Util::String l_FileName =
                  Util::PathHelper::get_base_name_no_ext(p_Path);
              Util::Serial::Node l_SidecarNode =
                  Util::Serial::load_yaml_file(p_Path.c_str());
              ScriptAsset l_Asset =
                  deserialize(l_SidecarNode, Util::Handle::DEAD);

              return l_Asset.get_id();
            });
        l_Builder.add_raw_suffix(".as");
        l_Builder.add_import_directory(Util::get_project().dataPath,
                                       true, true);
        l_Builder.importer(
            [](const Util::String p_Path) -> Util::String {
              const Util::String l_FileName =
                  Util::PathHelper::get_base_name_no_ext(p_Path);
              ScriptAsset l_Asset =
                  ScriptAsset::make(LOW_NAME(l_FileName.c_str()));
              l_Asset.set_source_path(p_Path);
              Util::Serial::Node l_OutNode;
              l_Asset.serialize(l_OutNode);
              l_Asset.set_module(Module::find_by_name(N(low.misc)));

              Util::String l_SidecarPath =
                  Util::get_project().assetCachePath;
              l_SidecarPath +=
                  "/" +
                  Util::hash_to_string(l_Asset.get_unique_id()) +
                  ".script.yaml";
              Util::Serial::write_yaml_file(l_SidecarPath.c_str(),
                                            l_OutNode);
              return l_SidecarPath;
            });

        Util::AssetManager::register_asset_type(l_Builder.build());
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void Asset::cleanup()
      {
        Low::Util::List<Asset> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle Asset::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Asset Asset::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Asset l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Asset::ms_TypeId;

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

      Asset Asset::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Asset l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Asset::ms_TypeId;

        return l_Handle;
      }

      bool Asset::is_alive() const
      {
        if (m_Data.m_Type != Asset::ms_TypeId) {
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
        return m_Data.m_Type == Asset::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Asset::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle Asset::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      Asset Asset::find_by_name(Low::Util::Name p_Name)
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

      Asset Asset::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        LOW_ASSERT_WARN(false, "Not implemented");
        return 0;
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE
      }

      Asset Asset::duplicate(Asset p_Handle, Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle Asset::_duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name)
      {
        Asset l_Asset = p_Handle.get_id();
        return l_Asset.duplicate(p_Name);
      }

      void Asset::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        p_Node["name"] = get_name();
        p_Node["unique_id"] = Util::U64Id{get_unique_id()};
        p_Node["source"] = get_source_path();
        if (get_module().is_alive()) {
          p_Node["module"] = get_module().get_name();
        }
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Asset::serialize(Low::Util::Handle p_Handle,
                            Low::Util::Serial::Node &p_Node)
      {
        Asset l_Asset = p_Handle.get_id();
        l_Asset.serialize(p_Node);
      }

      Low::Util::Handle
      Asset::deserialize(Low::Util::Serial::Node &p_Node,
                         Low::Util::Handle p_Creator)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        Util::UniqueId l_HandleUniqueId = 0ull;
        if (p_Node["unique_id"]) {
          l_HandleUniqueId = p_Node["unique_id"].as<Util::U64Id>();
          ScriptAsset l_ExistingAsset =
              Util::find_handle_by_unique_id(l_HandleUniqueId);
          if (l_ExistingAsset.is_alive()) {
            return l_ExistingAsset;
          }
        }
        ScriptAsset l_Asset =
            make(p_Node["name"].as<Util::Name>(), l_HandleUniqueId);
        l_Asset.set_source_path(p_Node["source"].as<Util::String>());

        Module l_Module = Util::Handle::DEAD;
        if (p_Node["module"]) {
          l_Module =
              Module::find_by_name(p_Node["module"].as<Util::Name>());
        }

        if (!l_Module.is_alive()) {
          l_Module = Module::find_by_name(N(low.misc));
        }
        l_Asset.set_module(l_Module);

        return l_Asset;
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
      }

      void
      Asset::broadcast_observable(Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 Asset::observe(Low::Util::Name p_Observable,
                         Low::Util::Function<void(Low::Util::Handle,
                                                  Low::Util::Name)>
                             p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Asset::observe(Low::Util::Name p_Observable,
                         Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Asset::notify(Low::Util::Handle p_Observed,
                         Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Asset::_notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
      {
        Asset l_Asset = p_Observer.get_id();
        l_Asset.notify(p_Observed, p_Observable);
      }

      Low::Core::Scripting::Module Asset::get_module() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Asset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_module
        // LOW_CODEGEN::END::CUSTOM:GETTER_module

        return TYPE_SOA(Asset, module, Low::Core::Scripting::Module);
      }
      void Asset::set_module(Low::Core::Scripting::Module p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Asset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_module
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_module

        // Set new value
        TYPE_SOA(Asset, module, Low::Core::Scripting::Module) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_module
        // LOW_CODEGEN::END::CUSTOM:SETTER_module

        broadcast_observable(N(module));
      }

      Low::Util::String Asset::get_source_path() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Asset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_source_path
        // LOW_CODEGEN::END::CUSTOM:GETTER_source_path

        return TYPE_SOA(Asset, source_path, Low::Util::String);
      }
      void Asset::set_source_path(const char *p_Value)
      {
        Low::Util::String l_Val(p_Value);
        set_source_path(l_Val);
      }

      void Asset::set_source_path(Low::Util::String p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Asset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_source_path
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_source_path

        // Set new value
        TYPE_SOA(Asset, source_path, Low::Util::String) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_source_path
        // LOW_CODEGEN::END::CUSTOM:SETTER_source_path

        broadcast_observable(N(source_path));
      }

      Low::Util::UniqueId Asset::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Asset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(Asset, unique_id, Low::Util::UniqueId);
      }
      void Asset::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Asset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(Asset, unique_id, Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

        broadcast_observable(N(unique_id));
      }

      Low::Util::Name Asset::get_name() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Asset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Asset, name, Low::Util::Name);
      }
      void Asset::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Asset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(Asset, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      uint32_t Asset::create_instance(
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

      u32 Asset::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Asset.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Asset::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Asset::get_page_for_index(const u32 p_Index,
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

    } // namespace Scripting
  } // namespace Core
} // namespace Low
