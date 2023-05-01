#include "LowCoreMeshRenderer.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Core {
    namespace Component {
      const uint16_t MeshRenderer::TYPE_ID = 24;
      uint32_t MeshRenderer::ms_Capacity = 0u;
      uint8_t *MeshRenderer::ms_Buffer = 0;
      Low::Util::Instances::Slot *MeshRenderer::ms_Slots = 0;
      Low::Util::List<MeshRenderer> MeshRenderer::ms_LivingInstances =
          Low::Util::List<MeshRenderer>();

      MeshRenderer::MeshRenderer() : Low::Util::Handle(0ull)
      {
      }
      MeshRenderer::MeshRenderer(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      MeshRenderer::MeshRenderer(MeshRenderer &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      MeshRenderer MeshRenderer::make(Low::Core::Entity p_Entity)
      {
        uint32_t l_Index = create_instance();

        MeshRenderer l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = MeshRenderer::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, MeshRenderer, mesh, MeshAsset))
            MeshAsset();
        new (&ACCESSOR_TYPE_SOA(l_Handle, MeshRenderer, material, Material))
            Material();
        new (&ACCESSOR_TYPE_SOA(l_Handle, MeshRenderer, entity,
                                Low::Core::Entity)) Low::Core::Entity();

        l_Handle.set_entity(p_Entity);
        p_Entity.add_component(l_Handle);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void MeshRenderer::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // LOW_CODEGEN::END::CUSTOM:DESTROY

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
        _LOW_ASSERT(l_LivingInstanceFound);
      }

      void MeshRenderer::initialize()
      {
        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(MeshRenderer));

        initialize_buffer(&ms_Buffer, MeshRendererData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_MeshRenderer);
        LOW_PROFILE_ALLOC(type_slots_MeshRenderer);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(MeshRenderer);
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &MeshRenderer::is_alive;
        l_TypeInfo.destroy = &MeshRenderer::destroy;
        l_TypeInfo.serialize = &MeshRenderer::serialize;
        l_TypeInfo.deserialize = &MeshRenderer::deserialize;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &MeshRenderer::living_instances);
        l_TypeInfo.get_living_count = &MeshRenderer::living_count;
        l_TypeInfo.component = true;
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(mesh);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(MeshRendererData, mesh);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = MeshAsset::TYPE_ID;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshRenderer, mesh,
                                              MeshAsset);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, MeshRenderer, mesh, MeshAsset) =
                *(MeshAsset *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(material);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(MeshRendererData, material);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Material::TYPE_ID;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshRenderer, material,
                                              Material);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, MeshRenderer, material, Material) =
                *(Material *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(entity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(MeshRendererData, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::TYPE_ID;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshRenderer, entity,
                                              Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, MeshRenderer, entity,
                              Low::Core::Entity) = *(Low::Core::Entity *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void MeshRenderer::cleanup()
      {
        Low::Util::List<MeshRenderer> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_MeshRenderer);
        LOW_PROFILE_FREE(type_slots_MeshRenderer);
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

      void MeshRenderer::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        get_mesh().serialize(p_Node["mesh"]);
        get_material().serialize(p_Node["material"]);

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void MeshRenderer::serialize(Low::Util::Handle p_Handle,
                                   Low::Util::Yaml::Node &p_Node)
      {
        MeshRenderer l_MeshRenderer = p_Handle.get_id();
        l_MeshRenderer.serialize(p_Node);
      }

      Low::Util::Handle MeshRenderer::deserialize(Low::Util::Yaml::Node &p_Node,
                                                  Low::Util::Handle p_Creator)
      {
        MeshRenderer l_Handle = MeshRenderer::make(p_Creator.get_id());

        l_Handle.set_mesh(
            MeshAsset::deserialize(p_Node["mesh"], l_Handle.get_id()).get_id());
        l_Handle.set_material(
            Material::deserialize(p_Node["material"], l_Handle.get_id())
                .get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      MeshAsset MeshRenderer::get_mesh() const
      {
        LOW_ASSERT(is_alive(), "Cannot get property from dead handle");
        return TYPE_SOA(MeshRenderer, mesh, MeshAsset);
      }
      void MeshRenderer::set_mesh(MeshAsset p_Value)
      {
        LOW_ASSERT(is_alive(), "Cannot set property on dead handle");

        // Set new value
        TYPE_SOA(MeshRenderer, mesh, MeshAsset) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mesh
        // LOW_CODEGEN::END::CUSTOM:SETTER_mesh
      }

      Material MeshRenderer::get_material() const
      {
        LOW_ASSERT(is_alive(), "Cannot get property from dead handle");
        return TYPE_SOA(MeshRenderer, material, Material);
      }
      void MeshRenderer::set_material(Material p_Value)
      {
        LOW_ASSERT(is_alive(), "Cannot set property on dead handle");

        // Set new value
        TYPE_SOA(MeshRenderer, material, Material) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material
        // LOW_CODEGEN::END::CUSTOM:SETTER_material
      }

      Low::Core::Entity MeshRenderer::get_entity() const
      {
        LOW_ASSERT(is_alive(), "Cannot get property from dead handle");
        return TYPE_SOA(MeshRenderer, entity, Low::Core::Entity);
      }
      void MeshRenderer::set_entity(Low::Core::Entity p_Value)
      {
        LOW_ASSERT(is_alive(), "Cannot set property on dead handle");

        // Set new value
        TYPE_SOA(MeshRenderer, entity, Low::Core::Entity) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity
        // LOW_CODEGEN::END::CUSTOM:SETTER_entity
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
        uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

        uint8_t *l_NewBuffer = (uint8_t *)malloc(
            (l_Capacity + l_CapacityIncrease) * sizeof(MeshRendererData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(&l_NewBuffer[offsetof(MeshRendererData, mesh) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(MeshRendererData, mesh) * (l_Capacity)],
                 l_Capacity * sizeof(MeshAsset));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(MeshRendererData, material) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(MeshRendererData, material) * (l_Capacity)],
              l_Capacity * sizeof(Material));
        }
        {
          memcpy(&l_NewBuffer[offsetof(MeshRendererData, entity) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(MeshRendererData, entity) * (l_Capacity)],
                 l_Capacity * sizeof(Low::Core::Entity));
        }
        for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease;
             ++i) {
          l_NewSlots[i].m_Occupied = false;
          l_NewSlots[i].m_Generation = 0;
        }
        free(ms_Buffer);
        free(ms_Slots);
        ms_Buffer = l_NewBuffer;
        ms_Slots = l_NewSlots;
        ms_Capacity = l_Capacity + l_CapacityIncrease;

        LOW_LOG_DEBUG << "Auto-increased budget for MeshRenderer from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease) << LOW_LOG_END;
      }
    } // namespace Component
  }   // namespace Core
} // namespace Low
