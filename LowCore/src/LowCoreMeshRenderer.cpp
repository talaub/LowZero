#include "LowCoreMeshRenderer.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowCorePrefabInstance.h"
// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t MeshRenderer::TYPE_ID = 26;
      uint32_t MeshRenderer::ms_Capacity = 0u;
      uint8_t *MeshRenderer::ms_Buffer = 0;
      Low::Util::Instances::Slot *MeshRenderer::ms_Slots = 0;
      Low::Util::List<MeshRenderer> MeshRenderer::ms_LivingInstances =
          Low::Util::List<MeshRenderer>();

      MeshRenderer::MeshRenderer() : Low::Util::Handle(0ull)
      {
      }
      MeshRenderer::MeshRenderer(uint64_t p_Id)
          : Low::Util::Handle(p_Id)
      {
      }
      MeshRenderer::MeshRenderer(MeshRenderer &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle
      MeshRenderer::_make(Low::Util::Handle p_Entity)
      {
        Low::Core::Entity l_Entity = p_Entity.get_id();
        LOW_ASSERT(l_Entity.is_alive(),
                   "Cannot create component for dead entity");
        return make(l_Entity).get_id();
      }

      MeshRenderer MeshRenderer::make(Low::Core::Entity p_Entity)
      {
        uint32_t l_Index = create_instance();

        MeshRenderer l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = MeshRenderer::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, MeshRenderer, mesh,
                                Low::Core::MeshAsset))
            Low::Core::MeshAsset();
        new (&ACCESSOR_TYPE_SOA(l_Handle, MeshRenderer, material,
                                Low::Core::Material))
            Low::Core::Material();
        new (&ACCESSOR_TYPE_SOA(l_Handle, MeshRenderer, entity,
                                Low::Core::Entity))
            Low::Core::Entity();

        l_Handle.set_entity(p_Entity);
        p_Entity.add_component(l_Handle);

        ms_LivingInstances.push_back(l_Handle);

        l_Handle.set_unique_id(
            Low::Util::generate_unique_id(l_Handle.get_id()));
        Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                      l_Handle.get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void MeshRenderer::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        if (get_material().is_alive() && get_material().is_loaded()) {
          get_material().unload();
        }

        if (get_mesh().is_alive() && get_mesh().is_loaded()) {
          get_mesh().unload();
        }
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        Low::Util::remove_unique_id(get_unique_id());

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const MeshRenderer *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
            ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
            l_LivingInstanceFound = true;
            break;
          }
        }
      }

      void MeshRenderer::initialize()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(
            N(LowCore), N(MeshRenderer));

        initialize_buffer(&ms_Buffer, MeshRendererData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_MeshRenderer);
        LOW_PROFILE_ALLOC(type_slots_MeshRenderer);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(MeshRenderer);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &MeshRenderer::is_alive;
        l_TypeInfo.destroy = &MeshRenderer::destroy;
        l_TypeInfo.serialize = &MeshRenderer::serialize;
        l_TypeInfo.deserialize = &MeshRenderer::deserialize;
        l_TypeInfo.find_by_index = &MeshRenderer::_find_by_index;
        l_TypeInfo.make_default = nullptr;
        l_TypeInfo.make_component = &MeshRenderer::_make;
        l_TypeInfo.duplicate_default = nullptr;
        l_TypeInfo.duplicate_component = &MeshRenderer::_duplicate;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &MeshRenderer::living_instances);
        l_TypeInfo.get_living_count = &MeshRenderer::living_count;
        l_TypeInfo.component = true;
        l_TypeInfo.uiComponent = false;
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(mesh);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(MeshRendererData, mesh);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::MeshAsset::TYPE_ID;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            MeshRenderer l_Handle = p_Handle.get_id();
            l_Handle.get_mesh();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, MeshRenderer, mesh, Low::Core::MeshAsset);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            MeshRenderer l_Handle = p_Handle.get_id();
            l_Handle.set_mesh(*(Low::Core::MeshAsset *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(material);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(MeshRendererData, material);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Material::TYPE_ID;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            MeshRenderer l_Handle = p_Handle.get_id();
            l_Handle.get_material();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshRenderer,
                                              material,
                                              Low::Core::Material);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            MeshRenderer l_Handle = p_Handle.get_id();
            l_Handle.set_material(*(Low::Core::Material *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(entity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(MeshRendererData, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::TYPE_ID;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            MeshRenderer l_Handle = p_Handle.get_id();
            l_Handle.get_entity();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, MeshRenderer, entity, Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            MeshRenderer l_Handle = p_Handle.get_id();
            l_Handle.set_entity(*(Low::Core::Entity *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(MeshRendererData, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            MeshRenderer l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshRenderer,
                                              unique_id,
                                              Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void MeshRenderer::cleanup()
      {
        Low::Util::List<MeshRenderer> l_Instances =
            ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_MeshRenderer);
        LOW_PROFILE_FREE(type_slots_MeshRenderer);
      }

      Low::Util::Handle MeshRenderer::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      MeshRenderer MeshRenderer::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        MeshRenderer l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = MeshRenderer::TYPE_ID;

        return l_Handle;
      }

      bool MeshRenderer::is_alive() const
      {
        return m_Data.m_Type == MeshRenderer::TYPE_ID &&
               check_alive(ms_Slots, MeshRenderer::get_capacity());
      }

      uint32_t MeshRenderer::get_capacity()
      {
        return ms_Capacity;
      }

      MeshRenderer
      MeshRenderer::duplicate(Low::Core::Entity p_Entity) const
      {
        _LOW_ASSERT(is_alive());

        MeshRenderer l_Handle = make(p_Entity);
        if (get_mesh().is_alive()) {
          l_Handle.set_mesh(get_mesh());
        }
        if (get_material().is_alive()) {
          l_Handle.set_material(get_material());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      MeshRenderer MeshRenderer::duplicate(MeshRenderer p_Handle,
                                           Low::Core::Entity p_Entity)
      {
        return p_Handle.duplicate(p_Entity);
      }

      Low::Util::Handle
      MeshRenderer::_duplicate(Low::Util::Handle p_Handle,
                               Low::Util::Handle p_Entity)
      {
        MeshRenderer l_MeshRenderer = p_Handle.get_id();
        Low::Core::Entity l_Entity = p_Entity.get_id();
        return l_MeshRenderer.duplicate(l_Entity);
      }

      void
      MeshRenderer::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["unique_id"] = get_unique_id();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        p_Node["mesh"] = get_mesh().get_unique_id();
        p_Node["material"] = get_material().get_unique_id();
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void MeshRenderer::serialize(Low::Util::Handle p_Handle,
                                   Low::Util::Yaml::Node &p_Node)
      {
        MeshRenderer l_MeshRenderer = p_Handle.get_id();
        l_MeshRenderer.serialize(p_Node);
      }

      Low::Util::Handle
      MeshRenderer::deserialize(Low::Util::Yaml::Node &p_Node,
                                Low::Util::Handle p_Creator)
      {
        MeshRenderer l_Handle =
            MeshRenderer::make(p_Creator.get_id());

        if (p_Node["unique_id"]) {
          Low::Util::remove_unique_id(l_Handle.get_unique_id());
          l_Handle.set_unique_id(p_Node["unique_id"].as<uint64_t>());
          Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                        l_Handle.get_id());
        }

        if (p_Node["unique_id"]) {
          l_Handle.set_unique_id(
              p_Node["unique_id"].as<Low::Util::UniqueId>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        l_Handle.set_mesh(Util::find_handle_by_unique_id(
                              p_Node["mesh"].as<uint64_t>())
                              .get_id());
        l_Handle.set_material(Util::find_handle_by_unique_id(
                                  p_Node["material"].as<uint64_t>())
                                  .get_id());
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      Low::Core::MeshAsset MeshRenderer::get_mesh() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mesh
        // LOW_CODEGEN::END::CUSTOM:GETTER_mesh

        return TYPE_SOA(MeshRenderer, mesh, Low::Core::MeshAsset);
      }
      void MeshRenderer::set_mesh(Low::Core::MeshAsset p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mesh
        if (get_mesh().is_alive() && get_mesh().is_loaded()) {
          get_mesh().unload();
        }
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_mesh

        // Set new value
        TYPE_SOA(MeshRenderer, mesh, Low::Core::MeshAsset) = p_Value;
        {
          Low::Core::Entity l_Entity = get_entity();
          if (l_Entity.has_component(
                  Low::Core::Component::PrefabInstance::TYPE_ID)) {
            Low::Core::Component::PrefabInstance l_Instance =
                l_Entity.get_component(
                    Low::Core::Component::PrefabInstance::TYPE_ID);
            Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
            if (l_Prefab.is_alive()) {
              l_Instance.override(
                  TYPE_ID, N(mesh),
                  !l_Prefab.compare_property(*this, N(mesh)));
            }
          }
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mesh
        if (p_Value.is_alive()) {
          p_Value.load();
        }
        // LOW_CODEGEN::END::CUSTOM:SETTER_mesh
      }

      Low::Core::Material MeshRenderer::get_material() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material
        // LOW_CODEGEN::END::CUSTOM:GETTER_material

        return TYPE_SOA(MeshRenderer, material, Low::Core::Material);
      }
      void MeshRenderer::set_material(Low::Core::Material p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material
        if (get_material().is_alive() && get_material().is_loaded()) {
          get_material().unload();
        }
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_material

        // Set new value
        TYPE_SOA(MeshRenderer, material, Low::Core::Material) =
            p_Value;
        {
          Low::Core::Entity l_Entity = get_entity();
          if (l_Entity.has_component(
                  Low::Core::Component::PrefabInstance::TYPE_ID)) {
            Low::Core::Component::PrefabInstance l_Instance =
                l_Entity.get_component(
                    Low::Core::Component::PrefabInstance::TYPE_ID);
            Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
            if (l_Prefab.is_alive()) {
              l_Instance.override(
                  TYPE_ID, N(material),
                  !l_Prefab.compare_property(*this, N(material)));
            }
          }
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material
        if (p_Value.is_alive()) {
          p_Value.load();
        }
        // LOW_CODEGEN::END::CUSTOM:SETTER_material
      }

      Low::Core::Entity MeshRenderer::get_entity() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity
        // LOW_CODEGEN::END::CUSTOM:GETTER_entity

        return TYPE_SOA(MeshRenderer, entity, Low::Core::Entity);
      }
      void MeshRenderer::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entity
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_entity

        // Set new value
        TYPE_SOA(MeshRenderer, entity, Low::Core::Entity) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity
        // LOW_CODEGEN::END::CUSTOM:SETTER_entity
      }

      Low::Util::UniqueId MeshRenderer::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(MeshRenderer, unique_id, Low::Util::UniqueId);
      }
      void MeshRenderer::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(MeshRenderer, unique_id, Low::Util::UniqueId) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
      }

      uint32_t MeshRenderer::create_instance()
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

      void MeshRenderer::increase_budget()
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
                              sizeof(MeshRendererData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(&l_NewBuffer[offsetof(MeshRendererData, mesh) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(MeshRendererData, mesh) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Core::MeshAsset));
        }
        {
          memcpy(&l_NewBuffer[offsetof(MeshRendererData, material) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(MeshRendererData, material) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Core::Material));
        }
        {
          memcpy(&l_NewBuffer[offsetof(MeshRendererData, entity) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(MeshRendererData, entity) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Core::Entity));
        }
        {
          memcpy(&l_NewBuffer[offsetof(MeshRendererData, unique_id) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(MeshRendererData, unique_id) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Util::UniqueId));
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

        LOW_LOG_DEBUG
            << "Auto-increased budget for MeshRenderer from "
            << l_Capacity << " to "
            << (l_Capacity + l_CapacityIncrease) << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Component
  }   // namespace Core
} // namespace Low
