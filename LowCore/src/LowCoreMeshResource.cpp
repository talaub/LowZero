#include "LowCoreMeshResource.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowUtilResource.h"
#include "LowRenderer.h"

namespace Low {
  namespace Core {
    const uint16_t MeshResource::TYPE_ID = 19;
    uint32_t MeshResource::ms_Capacity = 0u;
    uint8_t *MeshResource::ms_Buffer = 0;
    Low::Util::Instances::Slot *MeshResource::ms_Slots = 0;
    Low::Util::List<MeshResource> MeshResource::ms_LivingInstances =
        Low::Util::List<MeshResource>();

    MeshResource::MeshResource() : Low::Util::Handle(0ull)
    {
    }
    MeshResource::MeshResource(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    MeshResource::MeshResource(MeshResource &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    MeshResource MeshResource::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = create_instance();

      MeshResource l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = MeshResource::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshResource, path, Util::String))
          Util::String();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshResource, submeshes,
                              Util::List<Submesh>)) Util::List<Submesh>();
      ACCESSOR_TYPE_SOA(l_Handle, MeshResource, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

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
      _LOW_ASSERT(l_LivingInstanceFound);
    }

    void MeshResource::initialize()
    {
      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowCore), N(MeshResource));

      initialize_buffer(&ms_Buffer, MeshResourceData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_MeshResource);
      LOW_PROFILE_ALLOC(type_slots_MeshResource);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(MeshResource);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &MeshResource::is_alive;
      l_TypeInfo.destroy = &MeshResource::destroy;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &MeshResource::living_instances);
      l_TypeInfo.get_living_count = &MeshResource::living_count;
      l_TypeInfo.component = false;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MeshResourceData, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource, path,
                                            Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, MeshResource, path, Util::String) =
              *(Util::String *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(submeshes);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MeshResourceData, submeshes);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource, submeshes,
                                            Util::List<Submesh>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, MeshResource, submeshes,
                            Util::List<Submesh>) =
              *(Util::List<Submesh> *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(reference_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MeshResourceData, reference_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            reference_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, MeshResource, reference_count, uint32_t) =
              *(uint32_t *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MeshResourceData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, MeshResource, name, Low::Util::Name) =
              *(Low::Util::Name *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
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

    void MeshResource::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      p_Node["path"] = get_path().c_str();
      p_Node["name"] = get_name().c_str();
    }

    Util::String &MeshResource::get_path() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(MeshResource, path, Util::String);
    }
    void MeshResource::set_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(MeshResource, path, Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_path
    }

    Util::List<Submesh> &MeshResource::get_submeshes() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(MeshResource, submeshes, Util::List<Submesh>);
    }

    uint32_t MeshResource::get_reference_count() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(MeshResource, reference_count, uint32_t);
    }
    void MeshResource::set_reference_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(MeshResource, reference_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_reference_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_reference_count
    }

    Low::Util::Name MeshResource::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(MeshResource, name, Low::Util::Name);
    }
    void MeshResource::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(MeshResource, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    MeshResource MeshResource::make(Util::String &p_Path)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end();
           ++it) {
        if (it->get_path() == p_Path) {
          return *it;
        }
      }

      Util::String l_FileName = p_Path.substr(p_Path.find_last_of("/\\") + 1);
      MeshResource l_Mesh = MeshResource::make(LOW_NAME(l_FileName.c_str()));
      l_Mesh.set_path(p_Path);

      return l_Mesh;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    bool MeshResource::is_loaded()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_loaded
      return !get_submeshes().empty();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_loaded
    }

    void MeshResource::load()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_load
      LOW_ASSERT(is_alive(), "Mesh resource was not alive on load");
      LOW_ASSERT_WARN(!is_loaded(), "Trying to load already loaded mesh");

      Util::Resource::Mesh l_Mesh;
      Util::Resource::load_mesh(get_path(), l_Mesh);

      for (uint32_t i = 0u; i < l_Mesh.submeshes.size(); ++i) {
        for (uint32_t j = 0u; j < l_Mesh.submeshes[i].meshInfos.size(); ++j) {
          Submesh i_Submesh;
          i_Submesh.transformation = l_Mesh.submeshes[i].transform;
          i_Submesh.mesh = Renderer::upload_mesh(
              N(Submesh), l_Mesh.submeshes[i].meshInfos[j]);

          get_submeshes().push_back(i_Submesh);
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_load
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
      uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(MeshResourceData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData, path) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData, path) * (l_Capacity)],
               l_Capacity * sizeof(Util::String));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(MeshResourceData, submeshes) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() * sizeof(Util::List<Submesh>))])
              Util::List<Submesh>();
          *i_ValPtr = it->get_submeshes();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData, reference_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData, reference_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData, name) * (l_Capacity)],
               l_Capacity * sizeof(Low::Util::Name));
      }
      for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease; ++i) {
        l_NewSlots[i].m_Occupied = false;
        l_NewSlots[i].m_Generation = 0;
      }
      free(ms_Buffer);
      free(ms_Slots);
      ms_Buffer = l_NewBuffer;
      ms_Slots = l_NewSlots;
      ms_Capacity = l_Capacity + l_CapacityIncrease;

      LOW_LOG_DEBUG << "Auto-increased budget for MeshResource from "
                    << l_Capacity << " to " << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }
  } // namespace Core
} // namespace Low