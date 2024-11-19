#include "LowCoreMeshResource.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowUtilResource.h"
#include "LowUtilJobManager.h"
#include "LowRenderer.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

#define MESH_COUNT 50
    Util::List<bool> g_MeshSlots;
    Util::List<Util::Resource::Mesh> g_Meshes;

    struct MeshLoadSchedule
    {
      uint32_t meshIndex;
      Util::Future<void> future;
      MeshResource meshResource;

      MeshLoadSchedule(uint64_t p_Id, Util::Future<void> p_Future)
          : meshIndex(p_Id), future(std::move(p_Future))
      {
      }
    };

    Util::List<MeshLoadSchedule> g_MeshLoadSchedules;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t MeshResource::TYPE_ID = 21;
    uint32_t MeshResource::ms_Capacity = 0u;
    uint8_t *MeshResource::ms_Buffer = 0;
    Low::Util::Instances::Slot *MeshResource::ms_Slots = 0;
    Low::Util::List<MeshResource> MeshResource::ms_LivingInstances =
        Low::Util::List<MeshResource>();

    MeshResource::MeshResource() : Low::Util::Handle(0ull)
    {
    }
    MeshResource::MeshResource(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    MeshResource::MeshResource(MeshResource &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle MeshResource::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    MeshResource MeshResource::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = create_instance();

      MeshResource l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = MeshResource::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshResource, path,
                              Util::String)) Util::String();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshResource, submeshes,
                              Util::List<Submesh>))
          Util::List<Submesh>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshResource, skeleton,
                              Renderer::Skeleton))
          Renderer::Skeleton();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshResource, state,
                              ResourceState)) ResourceState();
      ACCESSOR_TYPE_SOA(l_Handle, MeshResource, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      l_Handle.set_reference_count(0);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void MeshResource::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const MeshResource *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void MeshResource::initialize()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      g_Meshes.resize(MESH_COUNT);
      g_MeshSlots.resize(MESH_COUNT);
      for (uint32_t i = 0; i < MESH_COUNT; ++i) {
        g_MeshSlots[i] = false;
      }
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowCore),
                                                    N(MeshResource));

      initialize_buffer(&ms_Buffer, MeshResourceData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_MeshResource);
      LOW_PROFILE_ALLOC(type_slots_MeshResource);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(MeshResource);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &MeshResource::is_alive;
      l_TypeInfo.destroy = &MeshResource::destroy;
      l_TypeInfo.serialize = &MeshResource::serialize;
      l_TypeInfo.deserialize = &MeshResource::deserialize;
      l_TypeInfo.find_by_index = &MeshResource::_find_by_index;
      l_TypeInfo.find_by_name = &MeshResource::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &MeshResource::_make;
      l_TypeInfo.duplicate_default = &MeshResource::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &MeshResource::living_instances);
      l_TypeInfo.get_living_count = &MeshResource::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MeshResourceData, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(submeshes);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResourceData, submeshes);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_submeshes();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshResource, submeshes, Util::List<Submesh>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(reference_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResourceData, reference_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          return nullptr;
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(skeleton);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResourceData, skeleton);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Renderer::Skeleton::TYPE_ID;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_skeleton();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshResource, skeleton, Renderer::Skeleton);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MeshResourceData, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            state, ResourceState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(ResourceState *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MeshResourceData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = MeshResource::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Path);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::STRING;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(is_loaded);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(load);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(_load);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_MeshIndex);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UINT32;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(unload);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(_unload);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(update);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void MeshResource::cleanup()
    {
      Low::Util::List<MeshResource> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_MeshResource);
      LOW_PROFILE_FREE(type_slots_MeshResource);
    }

    Low::Util::Handle MeshResource::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    MeshResource MeshResource::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      MeshResource l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = MeshResource::TYPE_ID;

      return l_Handle;
    }

    bool MeshResource::is_alive() const
    {
      return m_Data.m_Type == MeshResource::TYPE_ID &&
             check_alive(ms_Slots, MeshResource::get_capacity());
    }

    uint32_t MeshResource::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    MeshResource::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    MeshResource MeshResource::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    MeshResource MeshResource::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      MeshResource l_Handle = make(p_Name);
      l_Handle.set_path(get_path());
      l_Handle.set_reference_count(get_reference_count());
      if (get_skeleton().is_alive()) {
        l_Handle.set_skeleton(get_skeleton());
      }
      l_Handle.set_state(get_state());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    MeshResource MeshResource::duplicate(MeshResource p_Handle,
                                         Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    MeshResource::_duplicate(Low::Util::Handle p_Handle,
                             Low::Util::Name p_Name)
    {
      MeshResource l_MeshResource = p_Handle.get_id();
      return l_MeshResource.duplicate(p_Name);
    }

    void MeshResource::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      p_Node = get_path().c_str();
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void MeshResource::serialize(Low::Util::Handle p_Handle,
                                 Low::Util::Yaml::Node &p_Node)
    {
      MeshResource l_MeshResource = p_Handle.get_id();
      l_MeshResource.serialize(p_Node);
    }

    Low::Util::Handle
    MeshResource::deserialize(Low::Util::Yaml::Node &p_Node,
                              Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      MeshResource l_Resource =
          MeshResource::make(LOW_YAML_AS_STRING(p_Node));

      return l_Resource;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    Util::String &MeshResource::get_path() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path

      // LOW_CODEGEN::END::CUSTOM:GETTER_path

      return TYPE_SOA(MeshResource, path, Util::String);
    }
    void MeshResource::set_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

      // Set new value
      TYPE_SOA(MeshResource, path, Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path

      // LOW_CODEGEN::END::CUSTOM:SETTER_path
    }

    Util::List<Submesh> &MeshResource::get_submeshes() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_submeshes

      // LOW_CODEGEN::END::CUSTOM:GETTER_submeshes

      return TYPE_SOA(MeshResource, submeshes, Util::List<Submesh>);
    }

    uint32_t MeshResource::get_reference_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:GETTER_reference_count

      return TYPE_SOA(MeshResource, reference_count, uint32_t);
    }
    void MeshResource::set_reference_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_reference_count

      // Set new value
      TYPE_SOA(MeshResource, reference_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:SETTER_reference_count
    }

    Renderer::Skeleton MeshResource::get_skeleton() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_skeleton

      // LOW_CODEGEN::END::CUSTOM:GETTER_skeleton

      return TYPE_SOA(MeshResource, skeleton, Renderer::Skeleton);
    }
    void MeshResource::set_skeleton(Renderer::Skeleton p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_skeleton

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_skeleton

      // Set new value
      TYPE_SOA(MeshResource, skeleton, Renderer::Skeleton) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_skeleton

      // LOW_CODEGEN::END::CUSTOM:SETTER_skeleton
    }

    ResourceState MeshResource::get_state() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state

      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      return TYPE_SOA(MeshResource, state, ResourceState);
    }
    void MeshResource::set_state(ResourceState p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      TYPE_SOA(MeshResource, state, ResourceState) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state

      // LOW_CODEGEN::END::CUSTOM:SETTER_state
    }

    Low::Util::Name MeshResource::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(MeshResource, name, Low::Util::Name);
    }
    void MeshResource::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(MeshResource, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    MeshResource MeshResource::make(Util::String &p_Path)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_path() == p_Path) {
          return *it;
        }
      }

      Util::String l_FileName =
          p_Path.substr(p_Path.find_last_of("/\\") + 1);
      MeshResource l_Mesh =
          MeshResource::make(LOW_NAME(l_FileName.c_str()));
      l_Mesh.set_path(p_Path);

      return l_Mesh;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    bool MeshResource::is_loaded()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_loaded

      return get_state() == ResourceState::LOADED;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_loaded
    }

    void MeshResource::load()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_load

      LOW_ASSERT(is_alive(), "Mesh resource was not alive on load");

      set_reference_count(get_reference_count() + 1);

      LOW_ASSERT(get_reference_count() > 0,
                 "Increased MeshResource reference count, but its "
                 "not over 0. "
                 "Something went wrong.");

      if (get_state() != ResourceState::UNLOADED) {
        return;
      }

      set_state(ResourceState::STREAMING);

      Util::String l_P = get_path();
      uint32_t l_MeshIndex = 0;
      bool l_FoundIndex = false;

      do {
        for (uint32_t i = 0u; i < MESH_COUNT; ++i) {
          if (!g_MeshSlots[i]) {
            l_MeshIndex = i;
            l_FoundIndex = true;
            g_MeshSlots[i] = true;
            break;
          }
        }
      } while (!l_FoundIndex);

      Util::String l_FullPath =
          Util::get_project().dataPath + "\\resources\\meshes\\";
      l_FullPath += get_path().c_str();

      MeshLoadSchedule &l_LoadSchedule =
          g_MeshLoadSchedules.emplace_back(
              l_MeshIndex,
              Util::JobManager::default_pool().enqueue(
                  [l_FullPath, l_MeshIndex]() {
                    for (auto it = g_MeshLoadSchedules.begin();
                         it != g_MeshLoadSchedules.end(); ++it) {
                      if (it->meshIndex == l_MeshIndex) {
                        Util::Resource::load_mesh(
                            l_FullPath, g_Meshes[l_MeshIndex]);
                        break;
                      }
                    }
                  }));
      l_LoadSchedule.meshResource = *this;

      // LOW_CODEGEN::END::CUSTOM:FUNCTION_load
    }

    void MeshResource::_load(uint32_t p_MeshIndex)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION__load

      Util::Resource::Mesh &l_Mesh = g_Meshes[p_MeshIndex];

      for (uint32_t i = 0u; i < l_Mesh.submeshes.size(); ++i) {
        for (uint32_t j = 0u;
             j < l_Mesh.submeshes[i].meshInfos.size(); ++j) {
          Submesh i_Submesh;
          i_Submesh.name = l_Mesh.submeshes[i].name;
          i_Submesh.transformation = l_Mesh.submeshes[i].transform;
          i_Submesh.mesh = Renderer::upload_mesh(
              N(Submesh), l_Mesh.submeshes[i].meshInfos[j]);

          get_submeshes().push_back(i_Submesh);
        }
      }

      if (!l_Mesh.animations.empty()) {
        set_skeleton(Renderer::upload_skeleton(N(Skeleton), l_Mesh));
      }

      set_state(ResourceState::LOADED);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION__load
    }

    void MeshResource::unload()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_unload

      set_reference_count(get_reference_count() - 1);

      LOW_ASSERT(get_reference_count() >= 0,
                 "MeshResource reference count < 0. Something "
                 "went wrong.");

      if (get_reference_count() <= 0) {
        _unload();
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_unload
    }

    void MeshResource::_unload()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION__unload

      if (!is_loaded()) {
        return;
      }
      for (auto it = get_submeshes().begin();
           it != get_submeshes().end(); ++it) {
        Renderer::unload_mesh(it->mesh);
      }

      get_submeshes().clear();

      if (get_skeleton().is_alive()) {
        Renderer::unload_skeleton(get_skeleton());
      }
      set_state(ResourceState::UNLOADED);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION__unload
    }

    void MeshResource::update()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_update

      LOW_PROFILE_CPU("Core", "Update MeshResource");
      for (auto it = g_MeshLoadSchedules.begin();
           it != g_MeshLoadSchedules.end();) {
        if (it->future.wait_for(std::chrono::seconds(0)) ==
            std::future_status::ready) {
          it->meshResource._load(it->meshIndex);
          g_MeshSlots[it->meshIndex] = false;

          it = g_MeshLoadSchedules.erase(it);
        } else {
          ++it;
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_update
    }

    uint32_t MeshResource::create_instance()
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

    void MeshResource::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer =
          (uint8_t *)malloc((l_Capacity + l_CapacityIncrease) *
                            sizeof(MeshResourceData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData, path) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData, path) *
                          (l_Capacity)],
               l_Capacity * sizeof(Util::String));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(MeshResourceData, submeshes) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Util::List<Submesh>))])
              Util::List<Submesh>();
          *i_ValPtr = it->get_submeshes();
        }
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(MeshResourceData, reference_count) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MeshResourceData, reference_count) *
                       (l_Capacity)],
            l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData, skeleton) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData, skeleton) *
                          (l_Capacity)],
               l_Capacity * sizeof(Renderer::Skeleton));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData, state) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData, state) *
                          (l_Capacity)],
               l_Capacity * sizeof(ResourceState));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData, name) *
                          (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for MeshResource from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Core
} // namespace Low
