#include "LowCoreNavigationWorld.h"

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
#include "LowCoreNavigationBackend.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Navigation {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      static Low::Util::Name setting_name(const char *p_Name)
      {
        Low::Util::String l_FullName = "navigation/";
        l_FullName += p_Name;
        return LOW_NAME(l_FullName.c_str());
      }

      BuildSettings get_project_build_settings()
      {
        BuildSettings l_Settings;
        const Low::Util::ConfigSettings &l_ProjectSettings =
            Low::Util::get_project().settings;

        l_Settings.cell_size = l_ProjectSettings.get_float(
            setting_name("cell_size"), l_Settings.cell_size);
        l_Settings.cell_height = l_ProjectSettings.get_float(
            setting_name("cell_height"), l_Settings.cell_height);
        l_Settings.agent_height = l_ProjectSettings.get_float(
            setting_name("agent_height"), l_Settings.agent_height);
        l_Settings.agent_radius = l_ProjectSettings.get_float(
            setting_name("agent_radius"), l_Settings.agent_radius);
        l_Settings.agent_max_climb = l_ProjectSettings.get_float(
            setting_name("agent_max_climb"),
            l_Settings.agent_max_climb);
        l_Settings.agent_max_slope = l_ProjectSettings.get_float(
            setting_name("agent_max_slope"),
            l_Settings.agent_max_slope);
        l_Settings.tile_size =
            static_cast<int>(l_ProjectSettings.get_u32(
                setting_name("tile_size"),
                static_cast<u32>(l_Settings.tile_size)));

        return l_Settings;
      }

      bool save_project_build_settings(
          const BuildSettings &p_BuildSettings)
      {
        Low::Util::ConfigSettings &l_ProjectSettings =
            Low::Util::get_project_settings();

        l_ProjectSettings.set_float(setting_name("cell_size"),
                                    p_BuildSettings.cell_size);
        l_ProjectSettings.set_float(setting_name("cell_height"),
                                    p_BuildSettings.cell_height);
        l_ProjectSettings.set_float(setting_name("agent_height"),
                                    p_BuildSettings.agent_height);
        l_ProjectSettings.set_float(setting_name("agent_radius"),
                                    p_BuildSettings.agent_radius);
        l_ProjectSettings.set_float(setting_name("agent_max_climb"),
                                    p_BuildSettings.agent_max_climb);
        l_ProjectSettings.set_float(setting_name("agent_max_slope"),
                                    p_BuildSettings.agent_max_slope);
        l_ProjectSettings.set_u32(
            setting_name("tile_size"),
            static_cast<u32>(p_BuildSettings.tile_size));

        return Low::Util::save_project_settings();
      }

      void apply_build_settings(World p_World,
                                const BuildSettings &p_BuildSettings)
      {
        LOW_ASSERT(
            p_World.is_alive(),
            "Cannot apply build settings to dead navigation world");

        set_world_build_settings(
            static_cast<WorldBackend *>(p_World.get_world_ptr()),
            p_BuildSettings);
      }

      BuildSettings get_build_settings(World p_World)
      {
        LOW_ASSERT(
            p_World.is_alive(),
            "Cannot get build settings from dead navigation world");

        return get_world_build_settings(
            static_cast<WorldBackend *>(p_World.get_world_ptr()));
      }

      void clear_tile_registry(World p_World)
      {
        LOW_ASSERT(
            p_World.is_alive(),
            "Cannot clear tile registry on dead navigation world");

        Navigation::clear_tile_registry(
            static_cast<WorldBackend *>(p_World.get_world_ptr()));
      }

      bool ensure_tile(World p_World, TileCoord p_Coord, float p_MinY,
                       float p_MaxY, Tile *p_Tile)
      {
        LOW_ASSERT(p_World.is_alive(),
                   "Cannot ensure tile on dead navigation world");

        return Navigation::ensure_tile(
            static_cast<WorldBackend *>(p_World.get_world_ptr()),
            p_Coord, p_MinY, p_MaxY, p_Tile);
      }

      bool set_tile_state(World p_World, TileCoord p_Coord,
                          TileState p_State)
      {
        LOW_ASSERT(p_World.is_alive(),
                   "Cannot set tile state on dead navigation world");

        return Navigation::set_tile_state(
            static_cast<WorldBackend *>(p_World.get_world_ptr()),
            p_Coord, p_State);
      }

      bool get_tile(World p_World, TileCoord p_Coord, Tile *p_Tile)
      {
        LOW_ASSERT(p_World.is_alive(),
                   "Cannot get tile from dead navigation world");

        return Navigation::get_tile(
            static_cast<WorldBackend *>(p_World.get_world_ptr()),
            p_Coord, p_Tile);
      }

      void collect_tiles(World p_World,
                         Low::Util::List<Tile> *p_Tiles)
      {
        LOW_ASSERT(p_World.is_alive(),
                   "Cannot collect tiles from dead navigation world");

        Navigation::collect_tiles(
            static_cast<WorldBackend *>(p_World.get_world_ptr()),
            p_Tiles);
      }

      void collect_dirty_tiles(World p_World,
                               Low::Util::List<Tile> *p_Tiles)
      {
        LOW_ASSERT(
            p_World.is_alive(),
            "Cannot collect dirty tiles from dead navigation world");

        Navigation::collect_dirty_tiles(
            static_cast<WorldBackend *>(p_World.get_world_ptr()),
            p_Tiles);
      }

      bool remove_tile(World p_World, TileCoord p_Coord)
      {
        LOW_ASSERT(p_World.is_alive(),
                   "Cannot remove tile from dead navigation world");

        return Navigation::remove_tile(
            static_cast<WorldBackend *>(p_World.get_world_ptr()),
            p_Coord);
      }

      bool queue_tile(World p_World, TileCoord p_Coord)
      {
        LOW_ASSERT(p_World.is_alive(),
                   "Cannot queue tile on dead navigation world");

        return Navigation::queue_tile(
            static_cast<WorldBackend *>(p_World.get_world_ptr()),
            p_Coord);
      }

      uint32_t get_queued_tile_count(World p_World)
      {
        LOW_ASSERT(
            p_World.is_alive(),
            "Cannot count queued tiles on dead navigation world");

        return Navigation::get_queued_tile_count(
            static_cast<WorldBackend *>(p_World.get_world_ptr()));
      }

      uint64_t get_navmesh_revision(World p_World)
      {
        LOW_ASSERT(
            p_World.is_alive(),
            "Cannot get navmesh revision from dead navigation world");

        return Navigation::get_navmesh_revision(
            static_cast<WorldBackend *>(p_World.get_world_ptr()));
      }

      uint32_t update_tile_builds(World p_World,
                                  uint32_t p_MaxTilesToBuild)
      {
        LOW_ASSERT(
            p_World.is_alive(),
            "Cannot update tile builds on dead navigation world");

        uint32_t l_BuiltTileCount = 0u;
        WorldBackend *l_Backend =
            static_cast<WorldBackend *>(p_World.get_world_ptr());
        while (l_BuiltTileCount < p_MaxTilesToBuild) {
          TileCoord i_Coord;
          if (!Navigation::pop_next_queued_tile(l_Backend,
                                                &i_Coord)) {
            break;
          }

          Tile i_Tile;
          if (!get_tile(p_World, i_Coord, &i_Tile) ||
              (i_Tile.state != TileState::Queued &&
               i_Tile.state != TileState::Dirty)) {
            continue;
          }

          build_tile(p_World, i_Coord);
          ++l_BuiltTileCount;
        }

        return l_BuiltTileCount;
      }

      bool build_tile_from_geometry(World p_World, TileCoord p_Coord,
                                    const BuildGeometry &p_Geometry)
      {
        LOW_ASSERT(p_World.is_alive(),
                   "Cannot build tile on dead navigation world");

        return Navigation::build_navmesh_tile_from_geometry(
            static_cast<WorldBackend *>(p_World.get_world_ptr()),
            p_Coord, p_Geometry);
      }
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 World::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          World::IDENTIFIER(LOW_NAME(1181529166),
                            LOW_NAME(433497520));
      uint32_t World::ms_Capacity = 0u;
      uint32_t World::ms_PageSize = 0u;
      Low::Util::List<World> World::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *> World::ms_Pages;

      Low::Util::Handle World::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      World World::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        World l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = World::ms_TypeId;

        ACCESSOR_TYPE_SOA(l_Handle, World, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        l_Handle.set_world_ptr(
            create_world_backend(get_project_build_settings()));
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void World::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          destroy_world_backend(
              static_cast<WorldBackend *>(get_world_ptr()));
          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        _LOW_ASSERT(get_page_for_index(get_index(), l_PageIndex,
                                       l_SlotIndex));
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

        l_Page->slots[l_SlotIndex].m_Occupied = false;
        l_Page->slots[l_SlotIndex].m_Generation++;

        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end();) {
          if (it->get_id() == get_id()) {
            it = ms_LivingInstances.erase(it);
          } else {
            it++;
          }
        }
      }

      void World::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(
            N(LowCore), N(NavigationWorld));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(World));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, World::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(World);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &World::is_alive;
        l_TypeInfo.destroy = &World::destroy;
        l_TypeInfo.serialize = &World::serialize;
        l_TypeInfo.deserialize = &World::deserialize;
        l_TypeInfo.find_by_index = &World::_find_by_index;
        l_TypeInfo.notify = &World::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &World::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &World::_make;
        l_TypeInfo.duplicate_default = &World::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &World::living_instances);
        l_TypeInfo.get_living_count = &World::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: world_ptr
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(world_ptr);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(World::Data, world_ptr);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            World l_Handle = p_Handle.get_id();
            l_Handle.get_world_ptr();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, World,
                                              world_ptr, void *);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            World l_Handle = p_Handle.get_id();
            *((void **)p_Data) = l_Handle.get_world_ptr();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: world_ptr
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(World::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            World l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, World, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            World l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            World l_Handle = p_Handle.get_id();
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Function: build_from_geometry
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(build_from_geometry);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Geometry);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: build_from_geometry
        }
        {
          // Function: find_nearest_point
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(find_nearest_point);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Position);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR3;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_HalfExtents);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR3;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Result);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: find_nearest_point
        }
        {
          // Function: find_path
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(find_path);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Start);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR3;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_End);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR3;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_HalfExtents);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR3;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Result);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: find_path
        }
        {
          // Function: get_navmesh_revision
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_navmesh_revision);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_navmesh_revision
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void World::cleanup()
      {
        Low::Util::List<World> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
          Low::Util::Instances::Page *i_Page = *it;
          free(i_Page->buffer);
          free(i_Page->slots);
          delete i_Page;
          it = ms_Pages.erase(it);
        }

        ms_Capacity = 0;
      }

      Low::Util::Handle World::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      World World::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        World l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = World::ms_TypeId;

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
          l_Handle.m_Data.m_Generation = 0;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        l_Handle.m_Data.m_Generation =
            l_Page->slots[l_SlotIndex].m_Generation;

        return l_Handle;
      }

      World World::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        World l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = World::ms_TypeId;

        return l_Handle;
      }

      bool World::is_alive() const
      {
        if (m_Data.m_Type != World::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == World::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t World::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle World::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      World World::find_by_name(Low::Util::Name p_Name)
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

      World World::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        LOW_ASSERT_WARN(false, "Not implemented");
        return 0;
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE
      }

      World World::duplicate(World p_Handle, Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle World::_duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name)
      {
        World l_World = p_Handle.get_id();
        return l_World.duplicate(p_Name);
      }

      void World::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void World::serialize(Low::Util::Handle p_Handle,
                            Low::Util::Serial::Node &p_Node)
      {
        World l_World = p_Handle.get_id();
        l_World.serialize(p_Node);
      }

      Low::Util::Handle
      World::deserialize(Low::Util::Serial::Node &p_Node,
                         Low::Util::Handle p_Creator)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        return Low::Util::Handle::DEAD;
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
      }

      void
      World::broadcast_observable(Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 World::observe(Low::Util::Name p_Observable,
                         Low::Util::Function<void(Low::Util::Handle,
                                                  Low::Util::Name)>
                             p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 World::observe(Low::Util::Name p_Observable,
                         Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void World::notify(Low::Util::Handle p_Observed,
                         Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void World::_notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
      {
        World l_World = p_Observer.get_id();
        l_World.notify(p_Observed, p_Observable);
      }

      void *World::get_world_ptr() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_ptr
        // LOW_CODEGEN::END::CUSTOM:GETTER_world_ptr

        return TYPE_SOA(World, world_ptr, void *);
      }
      void World::set_world_ptr(void *p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_ptr
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_ptr

        // Set new value
        TYPE_SOA(World, world_ptr, void *) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_ptr
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_ptr

        broadcast_observable(N(world_ptr));
      }

      Low::Util::Name World::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(World, name, Low::Util::Name);
      }
      void World::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(World, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      bool World::build_from_geometry(
          Low::Core::Navigation::BuildGeometry *p_Geometry)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_build_from_geometry
        if (!p_Geometry) {
          return false;
        }
        return build_navmesh_from_geometry(
            static_cast<WorldBackend *>(get_world_ptr()),
            *p_Geometry);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_build_from_geometry
      }

      bool World::find_nearest_point(
          Low::Math::Vector3 p_Position,
          Low::Math::Vector3 p_HalfExtents,
          Low::Core::Navigation::NearestPointResult *p_Result)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_find_nearest_point
        if (!p_Result) {
          return false;
        }
        return Navigation::find_nearest_point(
            static_cast<WorldBackend *>(get_world_ptr()), p_Position,
            p_HalfExtents, *p_Result);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_find_nearest_point
      }

      bool
      World::find_path(Low::Math::Vector3 p_Start,
                       Low::Math::Vector3 p_End,
                       Low::Math::Vector3 p_HalfExtents,
                       Low::Core::Navigation::PathResult *p_Result)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_find_path
        if (!p_Result) {
          return false;
        }
        return Navigation::find_path(
            static_cast<WorldBackend *>(get_world_ptr()), p_Start,
            p_End, p_HalfExtents, *p_Result);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_find_path
      }

      uint64_t World::get_navmesh_revision()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_navmesh_revision
        return Navigation::get_navmesh_revision(
            static_cast<WorldBackend *>(get_world_ptr()));
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_navmesh_revision
      }

      uint32_t World::create_instance(u32 &p_PageIndex,
                                      u32 &p_SlotIndex)
      {
        u32 l_Index = 0;
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        bool l_FoundIndex = false;

        for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
             ++l_PageIndex) {
          for (l_SlotIndex = 0;
               l_SlotIndex < ms_Pages[l_PageIndex]->size;
               ++l_SlotIndex) {
            if (!ms_Pages[l_PageIndex]
                     ->slots[l_SlotIndex]
                     .m_Occupied) {
              l_FoundIndex = true;
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
        }
        ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
        p_PageIndex = l_PageIndex;
        p_SlotIndex = l_SlotIndex;
        return l_Index;
      }

      u32 World::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for World.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, World::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool World::get_page_for_index(const u32 p_Index,
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
      bool collect_navmesh_geometry(World p_World,
                                    BuildGeometry *p_Geometry)
      {
        if (!p_World.is_alive()) {
          return false;
        }

        return Navigation::collect_navmesh_geometry(
            static_cast<WorldBackend *>(p_World.get_world_ptr()),
            p_Geometry);
      }
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Navigation
  } // namespace Core
} // namespace Low
