#include "LowCorePhysicsShape.h"

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
#include "LowCorePhysicsBackend.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Physics {
// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
#define BACKEND_WORLD(p_World)                                       \
  static_cast<WorldBackend *>((p_World).get_world_ptr())
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 Shape::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          Shape::IDENTIFIER(LOW_NAME(1181529166),
                            LOW_NAME(485609692));
      uint32_t Shape::ms_Capacity = 0u;
      uint32_t Shape::ms_PageSize = 0u;
      Low::Util::List<Shape> Shape::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *> Shape::ms_Pages;

      Low::Util::Handle Shape::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Shape Shape::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        Shape l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Shape::ms_TypeId;

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Shape, world, World))
            World();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Shape, type, ShapeType))
            ShapeType();
        ACCESSOR_TYPE_SOA(l_Handle, Shape, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        l_Handle.set_backend_id(0u);
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Shape::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          if (get_backend_id() != 0u && get_world().is_alive()) {
            destroy_shape(BACKEND_WORLD(get_world()),
                          ShapeBackendHandle{get_backend_id()});
            set_backend_id(0u);
          }
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

      void Shape::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(N(LowCore),
                                                          N(Shape));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(Shape));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Shape::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Shape);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Shape::is_alive;
        l_TypeInfo.destroy = &Shape::destroy;
        l_TypeInfo.serialize = &Shape::serialize;
        l_TypeInfo.deserialize = &Shape::deserialize;
        l_TypeInfo.find_by_index = &Shape::_find_by_index;
        l_TypeInfo.notify = &Shape::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &Shape::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Shape::_make;
        l_TypeInfo.duplicate_default = &Shape::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Shape::living_instances);
        l_TypeInfo.get_living_count = &Shape::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: backend_id
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(backend_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Shape::Data, backend_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Shape l_Handle = p_Handle.get_id();
            l_Handle.get_backend_id();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Shape,
                                              backend_id, uint64_t);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Shape l_Handle = p_Handle.get_id();
            *((uint64_t *)p_Data) = l_Handle.get_backend_id();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: backend_id
        }
        {
          // Property: world
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(world);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Shape::Data, world);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = World::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Shape l_Handle = p_Handle.get_id();
            l_Handle.get_world();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Shape, world,
                                              World);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Shape l_Handle = p_Handle.get_id();
            *((World *)p_Data) = l_Handle.get_world();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: world
        }
        {
          // Property: type
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(type);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Shape::Data, type);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Shape l_Handle = p_Handle.get_id();
            l_Handle.get_type();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Shape, type,
                                              ShapeType);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Shape l_Handle = p_Handle.get_id();
            *((ShapeType *)p_Data) = l_Handle.get_type();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: type
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Shape::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Shape l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Shape, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Shape l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Shape l_Handle = p_Handle.get_id();
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
          l_FunctionInfo.handleType = Shape::type_id();
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_World);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = World::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Shape);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::SHAPE;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: make
        }
        {
          // Function: make_convex_hull
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(make_convex_hull);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType = Shape::type_id();
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_World);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = World::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Points);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: make_convex_hull
        }
        {
          // Function: debug_visualize
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(debug_visualize);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_RenderView);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType =
                Renderer::RenderView::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
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
            l_ParameterInfo.name = N(p_Rotation);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::QUATERNION;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Color);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::COLOR;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Wireframe);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::BOOL;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_DepthTest);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::BOOL;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: debug_visualize
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void Shape::cleanup()
      {
        Low::Util::List<Shape> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle Shape::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Shape Shape::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Shape l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Shape::ms_TypeId;

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

      Shape Shape::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Shape l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Shape::ms_TypeId;

        return l_Handle;
      }

      bool Shape::is_alive() const
      {
        if (m_Data.m_Type != Shape::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == Shape::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Shape::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle Shape::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      Shape Shape::find_by_name(Low::Util::Name p_Name)
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

      Shape Shape::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        LOW_ASSERT_WARN(false, "Not implemented");
        return 0;
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE
      }

      Shape Shape::duplicate(Shape p_Handle, Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle Shape::_duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name)
      {
        Shape l_Shape = p_Handle.get_id();
        return l_Shape.duplicate(p_Name);
      }

      void Shape::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Shape::serialize(Low::Util::Handle p_Handle,
                            Low::Util::Serial::Node &p_Node)
      {
        Shape l_Shape = p_Handle.get_id();
        l_Shape.serialize(p_Node);
      }

      Low::Util::Handle
      Shape::deserialize(Low::Util::Serial::Node &p_Node,
                         Low::Util::Handle p_Creator)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        return DEAD;
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
      }

      void
      Shape::broadcast_observable(Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 Shape::observe(Low::Util::Name p_Observable,
                         Low::Util::Function<void(Low::Util::Handle,
                                                  Low::Util::Name)>
                             p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Shape::observe(Low::Util::Name p_Observable,
                         Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Shape::notify(Low::Util::Handle p_Observed,
                         Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Shape::_notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
      {
        Shape l_Shape = p_Observer.get_id();
        l_Shape.notify(p_Observed, p_Observable);
      }

      uint64_t Shape::get_backend_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_backend_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_backend_id

        return TYPE_SOA(Shape, backend_id, uint64_t);
      }
      void Shape::set_backend_id(uint64_t p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_backend_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_backend_id

        // Set new value
        TYPE_SOA(Shape, backend_id, uint64_t) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_backend_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_backend_id

        broadcast_observable(N(backend_id));
      }

      World Shape::get_world() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world
        // LOW_CODEGEN::END::CUSTOM:GETTER_world

        return TYPE_SOA(Shape, world, World);
      }
      void Shape::set_world(World p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_world

        // Set new value
        TYPE_SOA(Shape, world, World) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world
        // LOW_CODEGEN::END::CUSTOM:SETTER_world

        broadcast_observable(N(world));
      }

      ShapeType Shape::get_type() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_type
        // LOW_CODEGEN::END::CUSTOM:GETTER_type

        return TYPE_SOA(Shape, type, ShapeType);
      }
      void Shape::set_type(ShapeType p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_type
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_type

        // Set new value
        TYPE_SOA(Shape, type, ShapeType) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_type
        // LOW_CODEGEN::END::CUSTOM:SETTER_type

        broadcast_observable(N(type));
      }

      Low::Util::Name Shape::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Shape, name, Low::Util::Name);
      }
      void Shape::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(Shape, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      Shape Shape::make(World p_World, Low::Math::Shape &p_Shape)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        _LOW_ASSERT(p_World.is_alive());

        ShapeCreateInfo l_CreateInfo;
        l_CreateInfo.shape = p_Shape;

        ShapeBackendHandle l_BackendHandle =
            create_shape(BACKEND_WORLD(p_World), l_CreateInfo);
        _LOW_ASSERT(l_BackendHandle.is_valid());

        Shape l_Shape = Shape::make(p_World.get_name());
        l_Shape.set_world(p_World);
        l_Shape.set_backend_id(l_BackendHandle.id);

        if (p_Shape.type == Math::ShapeType::SPHERE) {
          l_Shape.set_type(ShapeType::Sphere);
        } else if (p_Shape.type == Math::ShapeType::BOX) {
          l_Shape.set_type(ShapeType::Box);
        } else {
          LOW_ASSERT(false, "Unknown type");
        }

        return l_Shape;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      Shape Shape::make_convex_hull(
          World p_World, const Util::List<Math::Vector3> &p_Points)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_convex_hull
        _LOW_ASSERT(p_World.is_alive());
        _LOW_ASSERT(!p_Points.empty());

        ShapeBackendHandle l_BackendHandle = create_convex_hull_shape(
            BACKEND_WORLD(p_World), p_Points);
        _LOW_ASSERT(l_BackendHandle.is_valid());

        Shape l_Shape = Shape::make(p_World.get_name());
        l_Shape.set_world(p_World);
        l_Shape.set_backend_id(l_BackendHandle.id);

        l_Shape.set_type(ShapeType::ConvexHull);

        return l_Shape;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_convex_hull
      }

      void Shape::debug_visualize(Renderer::RenderView p_RenderView,
                                  Math::Vector3 p_Position,
                                  Math::Quaternion p_Rotation,
                                  Math::Color p_Color,
                                  bool p_Wireframe, bool p_DepthTest)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_debug_visualize
        if (get_type() == ShapeType::ConvexHull) {
          visualize_convex_hull(
              static_cast<WorldBackend *>(
                  get_world().get_world_ptr()),
              static_cast<ShapeBackendHandle>(get_backend_id()),
              p_RenderView, p_Position, p_Rotation, p_Color,
              p_Wireframe, p_DepthTest);
        }
        // TODO: Implement sphere + box
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_debug_visualize
      }

      uint32_t Shape::create_instance(u32 &p_PageIndex,
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

      u32 Shape::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Shape.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Shape::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Shape::get_page_for_index(const u32 p_Index,
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

    } // namespace Physics
  } // namespace Core
} // namespace Low
