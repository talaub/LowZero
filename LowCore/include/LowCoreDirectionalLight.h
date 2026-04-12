#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

#include "LowCoreEntity.h"

#include "LowMath.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API DirectionalLight : public Low::Util::Handle
      {
      public:
        struct Data
        {
        public:
          Low::Math::ColorRGB color;
          float intensity;
          Low::Core::Entity entity;
          Low::Util::UniqueId unique_id;

          static size_t get_size()
          {
            return sizeof(Data);
          }
        };

      private:
        static u16 ms_TypeId;

      public:
        static Low::Util::SharedMutex ms_LivingMutex;
        static Low::Util::UniqueLock<Low::Util::SharedMutex>
            ms_PagesLock;
        static Low::Util::SharedMutex ms_PagesMutex;
        static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

        static Low::Util::List<DirectionalLight> ms_LivingInstances;

        const static Low::Util::TypeIdentifier IDENTIFIER;

        [[nodiscard]] static u16 type_id()
        {
          return ms_TypeId;
        }

        static DirectionalLight make(Low::Core::Entity p_Entity);
        static Low::Util::Handle _make(Low::Util::Handle p_Entity);
        static DirectionalLight make(Low::Core::Entity p_Entity,
                                     Low::Util::UniqueId p_UniqueId);
        explicit DirectionalLight(const DirectionalLight &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        DirectionalLight(u64 p_Id) : Low::Util::Handle(p_Id)
        {
        }
        DirectionalLight() : Low::Util::Handle()
        {
        }
        DirectionalLight(Low::Util::Handle p_Handle)
            : Low::Util::Handle(p_Handle.get_id())
        {
        }

        using Handle::operator=;

        DirectionalLight &
        operator=(const DirectionalLight &) = default;
        DirectionalLight &
        operator=(DirectionalLight &&) noexcept = default;

        static uint32_t living_count()
        {
          Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static DirectionalLight *living_instances()
        {
          Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          return ms_LivingInstances.data();
        }

        static DirectionalLight create_handle_by_index(u32 p_Index);

        static DirectionalLight find_by_index(uint32_t p_Index);
        static Low::Util::Handle _find_by_index(uint32_t p_Index);

        bool is_alive() const;

        u64 observe(Low::Util::Name p_Observable,
                    Low::Util::Handle p_Observer) const;
        u64 observe(Low::Util::Name p_Observable,
                    Low::Util::Function<void(Low::Util::Handle,
                                             Low::Util::Name)>
                        p_Observer) const;
        void notify(Low::Util::Handle p_Observed,
                    Low::Util::Name p_Observable);
        void broadcast_observable(Low::Util::Name p_Observable) const;

        static void _notify(Low::Util::Handle p_Observer,
                            Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable);

        static uint32_t get_capacity();

        void serialize(Low::Util::Serial::Node &p_Node) const;

        DirectionalLight duplicate(Low::Core::Entity p_Entity) const;
        static DirectionalLight duplicate(DirectionalLight p_Handle,
                                          Low::Core::Entity p_Entity);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Handle p_Entity);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Serial::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Serial::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          DirectionalLight l_Handle = p_Handle.get_id();
          return l_Handle.is_alive();
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          DirectionalLight l_DirectionalLight = p_Handle.get_id();
          l_DirectionalLight.destroy();
        }

        Low::Math::ColorRGB get_color() const;
        void set_color(Low::Math::ColorRGB p_Value);
        void set_color(float p_X, float p_Y, float p_Z);
        void set_color_x(float p_Value);
        void set_color_y(float p_Value);
        void set_color_z(float p_Value);

        float get_intensity() const;
        void set_intensity(float p_Value);

        Low::Core::Entity get_entity() const;
        void set_entity(Low::Core::Entity p_Value);

        Low::Util::UniqueId get_unique_id() const;

        static bool get_page_for_index(const u32 p_Index,
                                       u32 &p_PageIndex,
                                       u32 &p_SlotIndex);

      private:
        static u32 ms_Capacity;
        static u32 ms_PageSize;
        static u32 create_instance(
            u32 &p_PageIndex, u32 &p_SlotIndex,
            Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock);
        static u32 create_page();
        void set_unique_id(Low::Util::UniqueId p_Value);

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE

        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Component
  } // namespace Core
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
