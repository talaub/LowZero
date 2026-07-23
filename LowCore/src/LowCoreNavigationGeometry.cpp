#include "LowCoreNavigation.h"

#include "LowCoreBoxCollider.h"
#include "LowCoreCharacterController.h"
#include "LowCoreConvexHullCollider.h"
#include "LowCoreDebugGeometry.h"
#include "LowCoreEntity.h"
#include "LowCoreNavigationInvoker.h"
#include "LowCoreNavigationSource.h"
#include "LowCoreNavigationWorld.h"
#include "LowCoreRegion.h"
#include "LowCoreScene.h"
#include "LowCoreSphereCollider.h"
#include "LowCoreTransform.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace Low {
  namespace Core {
    namespace Navigation {
      static bool g_BuildGeometryDebugRenderingEnabled = false;
      static bool g_NavmeshDebugRenderingEnabled = false;
      static bool g_TileDebugRenderingEnabled = false;
      static bool g_InvokerDebugRenderingEnabled = false;
      static std::vector<u64> g_SourcesBeingMarkedDirty;

      bool is_source_being_marked_dirty(Source p_Source)
      {
        const u64 l_SourceId = p_Source.get_id();
        return std::find(g_SourcesBeingMarkedDirty.begin(),
                         g_SourcesBeingMarkedDirty.end(),
                         l_SourceId) !=
               g_SourcesBeingMarkedDirty.end();
      }

      struct SourceDirtyGuard
      {
        u64 source_id;

        SourceDirtyGuard(Source p_Source)
            : source_id(p_Source.get_id())
        {
          g_SourcesBeingMarkedDirty.push_back(source_id);
        }

        ~SourceDirtyGuard()
        {
          g_SourcesBeingMarkedDirty.erase(
              std::remove(g_SourcesBeingMarkedDirty.begin(),
                          g_SourcesBeingMarkedDirty.end(), source_id),
              g_SourcesBeingMarkedDirty.end());
        }
      };

      TileCoord world_to_tile_coord(Math::Vector3 p_Position,
                                    float p_TileWorldSize)
      {
        LOW_ASSERT(p_TileWorldSize > 0.0f,
                   "Navigation tile world size must be positive");

        TileCoord l_Coord;
        l_Coord.x = static_cast<int>(
            std::floor(p_Position.x / p_TileWorldSize));
        l_Coord.z = static_cast<int>(
            std::floor(p_Position.z / p_TileWorldSize));
        return l_Coord;
      }

      Bounds tile_coord_to_bounds(TileCoord p_Coord,
                                  float p_TileWorldSize, float p_MinY,
                                  float p_MaxY)
      {
        LOW_ASSERT(p_TileWorldSize > 0.0f,
                   "Navigation tile world size must be positive");
        LOW_ASSERT(p_MinY <= p_MaxY,
                   "Navigation tile bounds Y range is invalid");

        Bounds l_Bounds;
        l_Bounds.min =
            Math::Vector3(static_cast<float>(p_Coord.x) *
                              p_TileWorldSize,
                          p_MinY,
                          static_cast<float>(p_Coord.z) *
                              p_TileWorldSize);
        l_Bounds.max =
            Math::Vector3(static_cast<float>(p_Coord.x + 1) *
                              p_TileWorldSize,
                          p_MaxY,
                          static_cast<float>(p_Coord.z + 1) *
                              p_TileWorldSize);
        return l_Bounds;
      }

      TileRange tile_range_for_bounds(const Bounds &p_Bounds,
                                      float p_TileWorldSize)
      {
        LOW_ASSERT(p_TileWorldSize > 0.0f,
                   "Navigation tile world size must be positive");
        LOW_ASSERT(p_Bounds.min.x <= p_Bounds.max.x &&
                       p_Bounds.min.y <= p_Bounds.max.y &&
                       p_Bounds.min.z <= p_Bounds.max.z,
                   "Navigation bounds are invalid");

        TileRange l_Range;
        l_Range.minimum =
            world_to_tile_coord(p_Bounds.min, p_TileWorldSize);
        l_Range.maximum =
            world_to_tile_coord(p_Bounds.max, p_TileWorldSize);
        return l_Range;
      }

      TileRange tile_range_for_radius(Math::Vector3 p_Position,
                                      float p_Radius,
                                      float p_TileWorldSize)
      {
        LOW_ASSERT(p_Radius >= 0.0f,
                   "Navigation tile radius must not be negative");

        Bounds l_Bounds;
        l_Bounds.min =
            p_Position - Math::Vector3(p_Radius, p_Radius, p_Radius);
        l_Bounds.max =
            p_Position + Math::Vector3(p_Radius, p_Radius, p_Radius);
        return tile_range_for_bounds(l_Bounds, p_TileWorldSize);
      }

      bool bounds_overlap(const Bounds &p_A, const Bounds &p_B)
      {
        return p_A.min.x <= p_B.max.x &&
               p_A.max.x >= p_B.min.x &&
               p_A.min.y <= p_B.max.y &&
               p_A.max.y >= p_B.min.y &&
               p_A.min.z <= p_B.max.z &&
               p_A.max.z >= p_B.min.z;
      }

      Bounds expand_bounds(const Bounds &p_Bounds, float p_Amount)
      {
        Bounds l_Bounds = p_Bounds;
        const Math::Vector3 l_Expansion(p_Amount);
        l_Bounds.min -= l_Expansion;
        l_Bounds.max += l_Expansion;
        return l_Bounds;
      }

      float get_tile_world_size(const BuildSettings &p_BuildSettings)
      {
        LOW_ASSERT(p_BuildSettings.cell_size > 0.0f,
                   "Navigation cell size must be positive");
        LOW_ASSERT(p_BuildSettings.tile_size > 0,
                   "Navigation tile size must be positive");

        return p_BuildSettings.cell_size *
               static_cast<float>(p_BuildSettings.tile_size);
      }

        static void include_point(BuildGeometry &p_Geometry,
                                  const Math::Vector3 &p_Point)
        {
          if (p_Geometry.vertices.size() == 1u) {
            p_Geometry.bounds.min = p_Point;
            p_Geometry.bounds.max = p_Point;
            return;
          }

          p_Geometry.bounds.min.x =
              std::min(p_Geometry.bounds.min.x, p_Point.x);
          p_Geometry.bounds.min.y =
              std::min(p_Geometry.bounds.min.y, p_Point.y);
          p_Geometry.bounds.min.z =
              std::min(p_Geometry.bounds.min.z, p_Point.z);
          p_Geometry.bounds.max.x =
              std::max(p_Geometry.bounds.max.x, p_Point.x);
          p_Geometry.bounds.max.y =
              std::max(p_Geometry.bounds.max.y, p_Point.y);
          p_Geometry.bounds.max.z =
              std::max(p_Geometry.bounds.max.z, p_Point.z);
        }

        static void include_point(Bounds &p_Bounds,
                                  bool &p_HasPoint,
                                  const Math::Vector3 &p_Point)
        {
          if (!p_HasPoint) {
            p_Bounds.min = p_Point;
            p_Bounds.max = p_Point;
            p_HasPoint = true;
            return;
          }

          p_Bounds.min.x = std::min(p_Bounds.min.x, p_Point.x);
          p_Bounds.min.y = std::min(p_Bounds.min.y, p_Point.y);
          p_Bounds.min.z = std::min(p_Bounds.min.z, p_Point.z);
          p_Bounds.max.x = std::max(p_Bounds.max.x, p_Point.x);
          p_Bounds.max.y = std::max(p_Bounds.max.y, p_Point.y);
          p_Bounds.max.z = std::max(p_Bounds.max.z, p_Point.z);
        }

        static void recompute_bounds(BuildGeometry &p_Geometry)
        {
          p_Geometry.bounds = Bounds();
          for (uint32_t i = 0u; i < p_Geometry.vertices.size(); ++i) {
            if (i == 0u) {
              p_Geometry.bounds.min = p_Geometry.vertices[i];
              p_Geometry.bounds.max = p_Geometry.vertices[i];
            } else {
              include_point(p_Geometry, p_Geometry.vertices[i]);
            }
          }
        }

        static void add_triangle(BuildGeometry &p_Geometry,
                                 uint32_t p_Index0, uint32_t p_Index1,
                                 uint32_t p_Index2,
                                 AreaType p_AreaType)
        {
          p_Geometry.indices.push_back(p_Index0);
          p_Geometry.indices.push_back(p_Index1);
          p_Geometry.indices.push_back(p_Index2);
          p_Geometry.triangle_area_types.push_back(p_AreaType);
        }

        static void
        add_box_geometry(BuildGeometry &p_Geometry,
                         Component::BoxCollider p_Collider,
                         Component::Transform p_Transform,
                         AreaType p_AreaType)
        {
          const Math::Vector3 l_WorldPosition =
              p_Transform.get_world_position() +
              (p_Transform.get_world_rotation() *
               p_Collider.get_center());
          const Math::Quaternion l_WorldRotation =
              p_Transform.get_world_rotation() *
              p_Collider.get_rotation();
          const Math::Vector3 l_WorldHalfExtents =
              p_Collider.get_half_extents() *
              glm::abs(p_Transform.get_world_scale());

          const Math::Vector3 l_LocalCorners[8] = {
              Math::Vector3(-l_WorldHalfExtents.x,
                            -l_WorldHalfExtents.y,
                            -l_WorldHalfExtents.z),
              Math::Vector3(l_WorldHalfExtents.x,
                            -l_WorldHalfExtents.y,
                            -l_WorldHalfExtents.z),
              Math::Vector3(l_WorldHalfExtents.x,
                            -l_WorldHalfExtents.y,
                            l_WorldHalfExtents.z),
              Math::Vector3(-l_WorldHalfExtents.x,
                            -l_WorldHalfExtents.y,
                            l_WorldHalfExtents.z),
              Math::Vector3(-l_WorldHalfExtents.x,
                            l_WorldHalfExtents.y,
                            -l_WorldHalfExtents.z),
              Math::Vector3(l_WorldHalfExtents.x,
                            l_WorldHalfExtents.y,
                            -l_WorldHalfExtents.z),
              Math::Vector3(l_WorldHalfExtents.x,
                            l_WorldHalfExtents.y,
                            l_WorldHalfExtents.z),
              Math::Vector3(-l_WorldHalfExtents.x,
                            l_WorldHalfExtents.y,
                            l_WorldHalfExtents.z)};

          const uint32_t l_BaseIndex =
              static_cast<uint32_t>(p_Geometry.vertices.size());
          for (const Math::Vector3 &i_LocalCorner : l_LocalCorners) {
            const Math::Vector3 l_WorldCorner =
                l_WorldPosition + (l_WorldRotation * i_LocalCorner);
            p_Geometry.vertices.push_back(l_WorldCorner);
            include_point(p_Geometry, l_WorldCorner);
          }

          add_triangle(p_Geometry, l_BaseIndex + 4u, l_BaseIndex + 6u,
                       l_BaseIndex + 5u, p_AreaType);
          add_triangle(p_Geometry, l_BaseIndex + 4u, l_BaseIndex + 7u,
                       l_BaseIndex + 6u, p_AreaType);

          add_triangle(p_Geometry, l_BaseIndex + 0u, l_BaseIndex + 1u,
                       l_BaseIndex + 2u, p_AreaType);
          add_triangle(p_Geometry, l_BaseIndex + 0u, l_BaseIndex + 2u,
                       l_BaseIndex + 3u, p_AreaType);

          add_triangle(p_Geometry, l_BaseIndex + 3u, l_BaseIndex + 2u,
                       l_BaseIndex + 6u, p_AreaType);
          add_triangle(p_Geometry, l_BaseIndex + 3u, l_BaseIndex + 6u,
                       l_BaseIndex + 7u, p_AreaType);

          add_triangle(p_Geometry, l_BaseIndex + 1u, l_BaseIndex + 0u,
                       l_BaseIndex + 4u, p_AreaType);
          add_triangle(p_Geometry, l_BaseIndex + 1u, l_BaseIndex + 4u,
                       l_BaseIndex + 5u, p_AreaType);

          add_triangle(p_Geometry, l_BaseIndex + 0u, l_BaseIndex + 3u,
                       l_BaseIndex + 7u, p_AreaType);
          add_triangle(p_Geometry, l_BaseIndex + 0u, l_BaseIndex + 7u,
                       l_BaseIndex + 4u, p_AreaType);

          add_triangle(p_Geometry, l_BaseIndex + 2u, l_BaseIndex + 1u,
                       l_BaseIndex + 5u, p_AreaType);
          add_triangle(p_Geometry, l_BaseIndex + 2u, l_BaseIndex + 5u,
                       l_BaseIndex + 6u, p_AreaType);
        }

        static bool
        compute_box_bounds(Bounds &p_Bounds,
                           Component::BoxCollider p_Collider,
                           Component::Transform p_Transform)
        {
          if (!p_Collider.is_alive() || p_Collider.is_trigger()) {
            return false;
          }

          const Math::Vector3 l_WorldPosition =
              p_Transform.get_world_position() +
              (p_Transform.get_world_rotation() *
               p_Collider.get_center());
          const Math::Quaternion l_WorldRotation =
              p_Transform.get_world_rotation() *
              p_Collider.get_rotation();
          const Math::Vector3 l_WorldHalfExtents =
              p_Collider.get_half_extents() *
              glm::abs(p_Transform.get_world_scale());

          const Math::Vector3 l_LocalCorners[8] = {
              Math::Vector3(-l_WorldHalfExtents.x,
                            -l_WorldHalfExtents.y,
                            -l_WorldHalfExtents.z),
              Math::Vector3(l_WorldHalfExtents.x,
                            -l_WorldHalfExtents.y,
                            -l_WorldHalfExtents.z),
              Math::Vector3(l_WorldHalfExtents.x,
                            -l_WorldHalfExtents.y,
                            l_WorldHalfExtents.z),
              Math::Vector3(-l_WorldHalfExtents.x,
                            -l_WorldHalfExtents.y,
                            l_WorldHalfExtents.z),
              Math::Vector3(-l_WorldHalfExtents.x,
                            l_WorldHalfExtents.y,
                            -l_WorldHalfExtents.z),
              Math::Vector3(l_WorldHalfExtents.x,
                            l_WorldHalfExtents.y,
                            -l_WorldHalfExtents.z),
              Math::Vector3(l_WorldHalfExtents.x,
                            l_WorldHalfExtents.y,
                            l_WorldHalfExtents.z),
              Math::Vector3(-l_WorldHalfExtents.x,
                            l_WorldHalfExtents.y,
                            l_WorldHalfExtents.z)};

          bool l_HasPoint = false;
          for (const Math::Vector3 &i_LocalCorner : l_LocalCorners) {
            include_point(p_Bounds, l_HasPoint,
                          l_WorldPosition +
                              (l_WorldRotation * i_LocalCorner));
          }
          return l_HasPoint;
        }

        static Math::Vector3
        transform_collider_point(Component::Transform p_Transform,
                                 const Math::Vector3 &p_LocalPoint)
        {
          return p_Transform.get_world_position() +
                 (p_Transform.get_world_rotation() * p_LocalPoint);
        }

        static void
        add_sphere_geometry(BuildGeometry &p_Geometry,
                            Component::SphereCollider p_Collider,
                            Component::Transform p_Transform,
                            AreaType p_AreaType)
        {
          constexpr uint32_t l_SegmentCount = 16u;
          constexpr float l_TwoPi = 6.28318530718f;

          const Math::Vector3 l_Scale =
              glm::abs(p_Transform.get_world_scale());
          const float l_Radius = p_Collider.get_radius() *
                                 std::max(l_Scale.x, l_Scale.z);
          const float l_HalfHeight =
              p_Collider.get_radius() * l_Scale.y;
          const Math::Vector3 l_Center = p_Collider.get_center();

          const uint32_t l_BaseIndex =
              static_cast<uint32_t>(p_Geometry.vertices.size());
          for (uint32_t i = 0u; i < l_SegmentCount; ++i) {
            const float i_Angle =
                (static_cast<float>(i) /
                 static_cast<float>(l_SegmentCount)) *
                l_TwoPi;
            const float i_X = std::cos(i_Angle) * l_Radius;
            const float i_Z = std::sin(i_Angle) * l_Radius;

            const Math::Vector3 i_Bottom = transform_collider_point(
                p_Transform,
                l_Center + Math::Vector3(i_X, -l_HalfHeight, i_Z));
            const Math::Vector3 i_Top = transform_collider_point(
                p_Transform,
                l_Center + Math::Vector3(i_X, l_HalfHeight, i_Z));

            p_Geometry.vertices.push_back(i_Bottom);
            include_point(p_Geometry, i_Bottom);
            p_Geometry.vertices.push_back(i_Top);
            include_point(p_Geometry, i_Top);
          }

          const uint32_t l_BottomCenter =
              static_cast<uint32_t>(p_Geometry.vertices.size());
          const Math::Vector3 l_BottomCenterPoint =
              transform_collider_point(
                  p_Transform,
                  l_Center +
                      Math::Vector3(0.0f, -l_HalfHeight, 0.0f));
          p_Geometry.vertices.push_back(l_BottomCenterPoint);
          include_point(p_Geometry, l_BottomCenterPoint);

          const uint32_t l_TopCenter =
              static_cast<uint32_t>(p_Geometry.vertices.size());
          const Math::Vector3 l_TopCenterPoint =
              transform_collider_point(
                  p_Transform,
                  l_Center + Math::Vector3(0.0f, l_HalfHeight, 0.0f));
          p_Geometry.vertices.push_back(l_TopCenterPoint);
          include_point(p_Geometry, l_TopCenterPoint);

          for (uint32_t i = 0u; i < l_SegmentCount; ++i) {
            const uint32_t i_Next = (i + 1u) % l_SegmentCount;
            const uint32_t i_Bottom = l_BaseIndex + (i * 2u);
            const uint32_t i_Top = i_Bottom + 1u;
            const uint32_t i_NextBottom = l_BaseIndex + (i_Next * 2u);
            const uint32_t i_NextTop = i_NextBottom + 1u;

            add_triangle(p_Geometry, i_Top, i_NextTop, i_NextBottom,
                         p_AreaType);
            add_triangle(p_Geometry, i_Top, i_NextBottom, i_Bottom,
                         p_AreaType);
            add_triangle(p_Geometry, l_TopCenter, i_NextTop, i_Top,
                         p_AreaType);
            add_triangle(p_Geometry, l_BottomCenter, i_Bottom,
                         i_NextBottom, p_AreaType);
          }
        }

        static bool
        compute_sphere_bounds(Bounds &p_Bounds,
                              Component::SphereCollider p_Collider,
                              Component::Transform p_Transform)
        {
          if (!p_Collider.is_alive() || p_Collider.is_trigger()) {
            return false;
          }

          constexpr uint32_t l_SegmentCount = 16u;
          constexpr float l_TwoPi = 6.28318530718f;
          const Math::Vector3 l_Scale =
              glm::abs(p_Transform.get_world_scale());
          const float l_Radius = p_Collider.get_radius() *
                                 std::max(l_Scale.x, l_Scale.z);
          const float l_HalfHeight =
              p_Collider.get_radius() * l_Scale.y;
          const Math::Vector3 l_Center = p_Collider.get_center();

          bool l_HasPoint = false;
          for (uint32_t i = 0u; i < l_SegmentCount; ++i) {
            const float i_Angle =
                (static_cast<float>(i) /
                 static_cast<float>(l_SegmentCount)) *
                l_TwoPi;
            const float i_X = std::cos(i_Angle) * l_Radius;
            const float i_Z = std::sin(i_Angle) * l_Radius;

            include_point(
                p_Bounds, l_HasPoint,
                transform_collider_point(
                    p_Transform,
                    l_Center +
                        Math::Vector3(i_X, -l_HalfHeight, i_Z)));
            include_point(
                p_Bounds, l_HasPoint,
                transform_collider_point(
                    p_Transform,
                    l_Center +
                        Math::Vector3(i_X, l_HalfHeight, i_Z)));
          }
          return l_HasPoint;
        }

        static void add_oriented_triangle(
            BuildGeometry &p_Geometry, uint32_t p_Index0,
            uint32_t p_Index1, uint32_t p_Index2,
            const Math::Vector3 &p_Normal, AreaType p_AreaType)
        {
          const Math::Vector3 &l_Vertex0 =
              p_Geometry.vertices[p_Index0];
          const Math::Vector3 &l_Vertex1 =
              p_Geometry.vertices[p_Index1];
          const Math::Vector3 &l_Vertex2 =
              p_Geometry.vertices[p_Index2];
          const Math::Vector3 l_Normal = glm::cross(
              l_Vertex1 - l_Vertex0, l_Vertex2 - l_Vertex0);

          if (glm::dot(l_Normal, p_Normal) < 0.0f) {
            add_triangle(p_Geometry, p_Index0, p_Index2, p_Index1,
                         p_AreaType);
          } else {
            add_triangle(p_Geometry, p_Index0, p_Index1, p_Index2,
                         p_AreaType);
          }
        }

        static bool add_convex_hull_geometry(
            BuildGeometry &p_Geometry,
            Component::ConvexHullCollider p_Collider,
            Component::Transform p_Transform, AreaType p_AreaType)
        {
          Low::Util::List<Math::Vector3> &l_Points =
              p_Collider.get_points();
          if (l_Points.size() < 4u) {
            return false;
          }

          const Math::Vector3 l_Scale = p_Transform.get_world_scale();
          const uint32_t l_BaseIndex =
              static_cast<uint32_t>(p_Geometry.vertices.size());
          const uint32_t l_BaseIndexCount =
              static_cast<uint32_t>(p_Geometry.indices.size());
          const uint32_t l_BaseAreaCount = static_cast<uint32_t>(
              p_Geometry.triangle_area_types.size());
          Math::Vector3 l_Centroid(0.0f);
          for (const Math::Vector3 &i_Point : l_Points) {
            const Math::Vector3 i_WorldPoint =
                transform_collider_point(p_Transform,
                                         i_Point * l_Scale);
            p_Geometry.vertices.push_back(i_WorldPoint);
            include_point(p_Geometry, i_WorldPoint);
            l_Centroid += i_WorldPoint;
          }
          l_Centroid /= static_cast<float>(l_Points.size());

          struct Plane
          {
            Math::Vector3 normal;
            float distance;
          };

          std::vector<Plane> l_Planes;
          constexpr float l_Epsilon = 0.0001f;
          for (uint32_t i = 0u; i < l_Points.size(); ++i) {
            for (uint32_t j = i + 1u; j < l_Points.size(); ++j) {
              for (uint32_t k = j + 1u; k < l_Points.size(); ++k) {
                const Math::Vector3 &i_Point0 =
                    p_Geometry.vertices[l_BaseIndex + i];
                const Math::Vector3 &i_Point1 =
                    p_Geometry.vertices[l_BaseIndex + j];
                const Math::Vector3 &i_Point2 =
                    p_Geometry.vertices[l_BaseIndex + k];
                Math::Vector3 i_Normal = glm::cross(
                    i_Point1 - i_Point0, i_Point2 - i_Point0);
                const float i_NormalLength = glm::length(i_Normal);
                if (i_NormalLength <= l_Epsilon) {
                  continue;
                }
                i_Normal /= i_NormalLength;

                bool i_HasPositive = false;
                bool i_HasNegative = false;
                const float i_Distance =
                    -glm::dot(i_Normal, i_Point0);
                for (uint32_t m = 0u; m < l_Points.size(); ++m) {
                  const float i_Side =
                      glm::dot(i_Normal,
                               p_Geometry.vertices[l_BaseIndex + m]) +
                      i_Distance;
                  if (i_Side > l_Epsilon) {
                    i_HasPositive = true;
                  } else if (i_Side < -l_Epsilon) {
                    i_HasNegative = true;
                  }
                }
                if (i_HasPositive && i_HasNegative) {
                  continue;
                }

                const Math::Vector3 i_FaceCenter =
                    (i_Point0 + i_Point1 + i_Point2) / 3.0f;
                if (glm::dot(i_Normal, i_FaceCenter - l_Centroid) <
                    0.0f) {
                  i_Normal *= -1.0f;
                }
                const float i_OrientedDistance =
                    -glm::dot(i_Normal, i_Point0);

                bool i_ExistingPlane = false;
                for (const Plane &i_Plane : l_Planes) {
                  if (glm::dot(i_Plane.normal, i_Normal) > 0.999f &&
                      std::fabs(i_Plane.distance -
                                i_OrientedDistance) < l_Epsilon) {
                    i_ExistingPlane = true;
                    break;
                  }
                }
                if (i_ExistingPlane) {
                  continue;
                }
                l_Planes.push_back(
                    Plane{i_Normal, i_OrientedDistance});
              }
            }
          }

          for (const Plane &i_Plane : l_Planes) {
            std::vector<uint32_t> i_FaceIndices;
            Math::Vector3 i_FaceCenter(0.0f);
            for (uint32_t i = 0u; i < l_Points.size(); ++i) {
              const Math::Vector3 &i_Point =
                  p_Geometry.vertices[l_BaseIndex + i];
              const float i_Side = glm::dot(i_Plane.normal, i_Point) +
                                   i_Plane.distance;
              if (std::fabs(i_Side) <= l_Epsilon) {
                i_FaceIndices.push_back(l_BaseIndex + i);
                i_FaceCenter += i_Point;
              }
            }

            if (i_FaceIndices.size() < 3u) {
              continue;
            }

            i_FaceCenter /= static_cast<float>(i_FaceIndices.size());
            const Math::Vector3 l_Reference =
                std::fabs(i_Plane.normal.y) < 0.9f
                    ? Math::Vector3(0.0f, 1.0f, 0.0f)
                    : Math::Vector3(1.0f, 0.0f, 0.0f);
            const Math::Vector3 l_Tangent = glm::normalize(
                glm::cross(l_Reference, i_Plane.normal));
            const Math::Vector3 l_Bitangent =
                glm::cross(i_Plane.normal, l_Tangent);

            std::sort(
                i_FaceIndices.begin(), i_FaceIndices.end(),
                [&](uint32_t p_Left, uint32_t p_Right) {
                  const Math::Vector3 l_Left =
                      p_Geometry.vertices[p_Left] - i_FaceCenter;
                  const Math::Vector3 l_Right =
                      p_Geometry.vertices[p_Right] - i_FaceCenter;
                  const float l_LeftAngle =
                      std::atan2(glm::dot(l_Left, l_Bitangent),
                                 glm::dot(l_Left, l_Tangent));
                  const float l_RightAngle =
                      std::atan2(glm::dot(l_Right, l_Bitangent),
                                 glm::dot(l_Right, l_Tangent));
                  return l_LeftAngle < l_RightAngle;
                });

            for (uint32_t i = 1u; i + 1u < i_FaceIndices.size();
                 ++i) {
              add_oriented_triangle(
                  p_Geometry, i_FaceIndices[0u], i_FaceIndices[i],
                  i_FaceIndices[i + 1u], i_Plane.normal, p_AreaType);
            }
          }

          if (p_Geometry.indices.size() == l_BaseIndexCount) {
            p_Geometry.vertices.resize(l_BaseIndex);
            p_Geometry.indices.resize(l_BaseIndexCount);
            p_Geometry.triangle_area_types.resize(l_BaseAreaCount);
            return false;
          }

          return true;
        }

        static bool compute_convex_hull_bounds(
            Bounds &p_Bounds,
            Component::ConvexHullCollider p_Collider,
            Component::Transform p_Transform)
        {
          if (!p_Collider.is_alive() || p_Collider.is_trigger()) {
            return false;
          }

          Low::Util::List<Math::Vector3> &l_Points =
              p_Collider.get_points();
          if (l_Points.empty()) {
            return false;
          }

          const Math::Vector3 l_Scale = p_Transform.get_world_scale();
          bool l_HasPoint = false;
          for (const Math::Vector3 &i_Point : l_Points) {
            include_point(
                p_Bounds, l_HasPoint,
                transform_collider_point(p_Transform,
                                         i_Point * l_Scale));
          }
          return l_HasPoint;
        }

        static bool resolve_source_collider(
            Source p_Source, Entity *p_Entity,
            Component::Transform *p_Transform, bool p_LogWarnings)
        {
          if (!p_Source.is_alive() ||
              p_Source.get_mode() == SourceMode::IGNORE) {
            return false;
          }

          Entity l_Entity = p_Source.get_entity();
          if (!l_Entity.is_alive()) {
            return false;
          }

          if (p_Source.get_mode() == SourceMode::MODIFIER) {
            if (p_LogWarnings) {
              LOW_LOG_WARN << "Navigation source on entity '"
                           << l_Entity.get_name()
                           << "' uses unsupported modifier mode."
                           << LOW_LOG_END;
            }
            return false;
          }

          if (p_Source.get_geometry_type() !=
              SourceGeometryType::COLLIDER) {
            if (p_LogWarnings) {
              LOW_LOG_WARN << "Navigation source on entity '"
                           << l_Entity.get_name()
                           << "' uses unsupported geometry type."
                           << LOW_LOG_END;
            }
            return false;
          }

          if (!l_Entity.has_component(
                  Component::BoxCollider::type_id()) &&
              !l_Entity.has_component(
                  Component::SphereCollider::type_id()) &&
              !l_Entity.has_component(
                  Component::ConvexHullCollider::type_id())) {
            if (p_LogWarnings) {
              if (l_Entity.has_component(
                      Component::CharacterController::type_id())) {
                LOW_LOG_WARN << "Navigation source on entity '"
                             << l_Entity.get_name()
                             << "' uses CharacterController geometry, "
                                "which is ignored for navmesh builds."
                             << LOW_LOG_END;
              } else {
                LOW_LOG_WARN
                    << "Navigation source on entity '"
                    << l_Entity.get_name()
                    << "' uses collider geometry but has no supported "
                       "collider component."
                    << LOW_LOG_END;
              }
            }
            return false;
          }

          Component::Transform l_Transform = l_Entity.get_transform();
          if (!l_Transform.is_alive()) {
            return false;
          }

          if (p_Entity) {
            *p_Entity = l_Entity;
          }
          if (p_Transform) {
            *p_Transform = l_Transform;
          }
          return true;
        }

        static bool compute_source_bounds(Source p_Source,
                                          Bounds *p_Bounds)
        {
          Entity l_Entity;
          Component::Transform l_Transform;
          if (!p_Bounds ||
              !resolve_source_collider(p_Source, &l_Entity,
                                       &l_Transform, false)) {
            return false;
          }

          *p_Bounds = Bounds();
          if (l_Entity.has_component(
                  Component::BoxCollider::type_id())) {
            return compute_box_bounds(
                *p_Bounds,
                l_Entity.get_component(
                    Component::BoxCollider::type_id()),
                l_Transform);
          }

          if (l_Entity.has_component(
                  Component::SphereCollider::type_id())) {
            return compute_sphere_bounds(
                *p_Bounds,
                l_Entity.get_component(
                    Component::SphereCollider::type_id()),
                l_Transform);
          }

          if (l_Entity.has_component(
                  Component::ConvexHullCollider::type_id())) {
            return compute_convex_hull_bounds(
                *p_Bounds,
                l_Entity.get_component(
                    Component::ConvexHullCollider::type_id()),
                l_Transform);
          }

          return false;
        }

        static bool source_belongs_to_world(Source p_Source,
                                            World p_World)
        {
          if (!p_Source.is_alive() || !p_World.is_alive()) {
            return false;
          }

          Entity l_Entity = p_Source.get_entity();
          if (!l_Entity.is_alive()) {
            return false;
          }

          Region l_Region = l_Entity.get_region();
          if (!l_Region.is_alive()) {
            return false;
          }

          Scene l_Scene = l_Region.get_scene();
          return l_Scene.is_alive() &&
                 l_Scene.get_navigation_world().get_id() ==
                     p_World.get_id();
        }

        struct InvokerInfo
        {
          uint64_t invoker_id = 0ull;
          Math::Vector3 position = Math::Vector3(0.0f);
          float generation_radius = 0.0f;
          float removal_radius = 0.0f;
        };

        struct InvokerTileCacheEntry
        {
          uint64_t invoker_id = 0ull;
          TileRange generation_range;
          TileRange removal_range;
          float generation_radius = 0.0f;
          float removal_radius = 0.0f;
          bool touched = false;
        };

        struct WorldInvokerTileCache
        {
          uint64_t world_id = 0ull;
          Low::Util::List<InvokerTileCacheEntry> entries;
          bool eviction_requested = false;
          uint32_t ticks_since_eviction = 0u;
        };

        static Low::Util::List<WorldInvokerTileCache>
            g_InvokerTileCaches;

        static bool tile_ranges_equal(const TileRange &p_A,
                                      const TileRange &p_B)
        {
          return p_A.minimum == p_B.minimum &&
                 p_A.maximum == p_B.maximum;
        }

        static WorldInvokerTileCache *
        get_world_invoker_tile_cache(World p_World)
        {
          const uint64_t l_WorldId = p_World.get_id();
          for (WorldInvokerTileCache &i_Cache :
               g_InvokerTileCaches) {
            if (i_Cache.world_id == l_WorldId) {
              return &i_Cache;
            }
          }

          WorldInvokerTileCache l_Cache;
          l_Cache.world_id = l_WorldId;
          g_InvokerTileCaches.push_back(l_Cache);
          return &g_InvokerTileCaches.back();
        }

        static InvokerTileCacheEntry *find_invoker_cache_entry(
            WorldInvokerTileCache *p_Cache, uint64_t p_InvokerId)
        {
          if (!p_Cache) {
            return nullptr;
          }

          for (InvokerTileCacheEntry &i_Entry :
               p_Cache->entries) {
            if (i_Entry.invoker_id == p_InvokerId) {
              return &i_Entry;
            }
          }
          return nullptr;
        }

        static void begin_invoker_cache_update(
            WorldInvokerTileCache *p_Cache)
        {
          if (!p_Cache) {
            return;
          }

          for (InvokerTileCacheEntry &i_Entry :
               p_Cache->entries) {
            i_Entry.touched = false;
          }
        }

        static void end_invoker_cache_update(
            WorldInvokerTileCache *p_Cache)
        {
          if (!p_Cache) {
            return;
          }

          for (auto it = p_Cache->entries.begin();
               it != p_Cache->entries.end();) {
            if (!it->touched) {
              p_Cache->eviction_requested = true;
              it = p_Cache->entries.erase(it);
            } else {
              ++it;
            }
          }
        }

        static bool collect_world_invokers(
            World p_World, Low::Util::List<InvokerInfo> *p_Invokers)
        {
          if (!p_World.is_alive() || !p_Invokers) {
            return false;
          }

          p_Invokers->clear();
          for (uint32_t i = 0u; i < Invoker::living_count(); ++i) {
            Invoker i_Invoker = Invoker::living_instances()[i];
            if (!i_Invoker.is_alive()) {
              continue;
            }

            Entity l_Entity = i_Invoker.get_entity();
            if (!l_Entity.is_alive()) {
              continue;
            }

            Region l_Region = l_Entity.get_region();
            if (!l_Region.is_alive()) {
              continue;
            }

            Scene l_Scene = l_Region.get_scene();
            if (!l_Scene.is_alive() ||
                l_Scene.get_navigation_world().get_id() !=
                    p_World.get_id()) {
              continue;
            }

            Component::Transform l_Transform =
                l_Entity.get_transform();
            if (!l_Transform.is_alive()) {
              continue;
            }

            InvokerInfo l_Info;
            l_Info.invoker_id = i_Invoker.get_unique_id();
            l_Info.position = l_Transform.get_world_position();
            l_Info.generation_radius =
                i_Invoker.get_generation_radius();
            l_Info.removal_radius = i_Invoker.get_removal_radius();
            p_Invokers->push_back(l_Info);
          }

          return !p_Invokers->empty();
        }

        static Math::Vector3 tile_center(const Tile &p_Tile)
        {
          return (p_Tile.bounds.min + p_Tile.bounds.max) * 0.5f;
        }

        static float distance_squared_xz(const Math::Vector3 &p_A,
                                         const Math::Vector3 &p_B)
        {
          const float l_X = p_A.x - p_B.x;
          const float l_Z = p_A.z - p_B.z;
          return (l_X * l_X) + (l_Z * l_Z);
        }

        static float nearest_invoker_distance_squared(
            const Tile &p_Tile,
            const Low::Util::List<InvokerInfo> &p_Invokers)
        {
          const Math::Vector3 l_Center = tile_center(p_Tile);
          float l_Nearest =
              std::numeric_limits<float>::max();
          for (const InvokerInfo &i_Invoker : p_Invokers) {
            l_Nearest = std::min(
                l_Nearest,
                distance_squared_xz(l_Center, i_Invoker.position));
          }
          return l_Nearest;
        }

        static bool tile_inside_any_invoker_removal_radius(
            const Tile &p_Tile,
            const Low::Util::List<InvokerInfo> &p_Invokers)
        {
          const Math::Vector3 l_Center = tile_center(p_Tile);
          for (const InvokerInfo &i_Invoker : p_Invokers) {
            if (i_Invoker.removal_radius < 0.0f) {
              continue;
            }

            const float l_RemovalRadiusSquared =
                i_Invoker.removal_radius * i_Invoker.removal_radius;
            if (distance_squared_xz(l_Center, i_Invoker.position) <=
                l_RemovalRadiusSquared) {
              return true;
            }
          }
          return false;
        }

        static Math::Color get_area_debug_color(AreaType p_AreaType)
        {
          switch (p_AreaType) {
          case AreaType::PREFERRED:
            return Math::Color(0.0f, 0.75f, 1.0f, 0.2f);
          case AreaType::NORMAL:
            return Math::Color(1.0f, 0.65f, 0.05f, 0.18f);
          case AreaType::ROUGH:
            return Math::Color(0.9f, 0.25f, 0.95f, 0.22f);
          case AreaType::DIFFICULT:
            return Math::Color(1.0f, 0.2f, 0.12f, 0.24f);
          case AreaType::BLOCKED:
            return Math::Color(0.35f, 0.35f, 0.35f, 0.18f);
          }

          return Math::Color(1.0f, 0.65f, 0.05f, 0.18f);
        }

        static Math::Color get_tile_debug_color(TileState p_State)
        {
          switch (p_State) {
          case TileState::Empty:
            return Math::Color(0.45f, 0.45f, 0.45f, 0.18f);
          case TileState::Dirty:
            return Math::Color(1.0f, 0.85f, 0.1f, 0.28f);
          case TileState::Queued:
            return Math::Color(0.25f, 0.55f, 1.0f, 0.28f);
          case TileState::Building:
            return Math::Color(0.75f, 0.35f, 1.0f, 0.32f);
          case TileState::Ready:
            return Math::Color(0.2f, 0.9f, 0.35f, 0.22f);
          case TileState::Failed:
            return Math::Color(1.0f, 0.15f, 0.1f, 0.35f);
          }

          return Math::Color(1.0f, 1.0f, 1.0f, 0.2f);
        }

        static bool append_source_geometry(BuildGeometry &p_Geometry,
                                           Source p_Source)
        {
          Entity l_Entity;
          Component::Transform l_Transform;
          if (!resolve_source_collider(p_Source, &l_Entity,
                                       &l_Transform, true)) {
            return false;
          }

          const AreaType l_AreaType =
              p_Source.get_mode() == SourceMode::OBSTACLE
                  ? AreaType::BLOCKED
                  : p_Source.get_area_type();

          if (p_Source.get_mode() == SourceMode::SURFACE &&
              l_AreaType == AreaType::BLOCKED) {
            LOW_LOG_WARN << "Navigation source on entity '"
                         << l_Entity.get_name()
                         << "' uses blocked area on a surface. Use "
                            "obstacle mode for blockers."
                         << LOW_LOG_END;
          }

          if (l_Entity.has_component(
                  Component::BoxCollider::type_id())) {
            Component::BoxCollider l_Collider =
                l_Entity.get_component(
                    Component::BoxCollider::type_id());
            if (!l_Collider.is_alive() || l_Collider.is_trigger()) {
              return false;
            }

            add_box_geometry(p_Geometry, l_Collider, l_Transform,
                             l_AreaType);
            return true;
          }

          if (l_Entity.has_component(
                  Component::SphereCollider::type_id())) {
            Component::SphereCollider l_Collider =
                l_Entity.get_component(
                    Component::SphereCollider::type_id());
            if (!l_Collider.is_alive() || l_Collider.is_trigger()) {
              return false;
            }

            add_sphere_geometry(p_Geometry, l_Collider, l_Transform,
                                l_AreaType);
            return true;
          }

          Component::ConvexHullCollider l_Collider =
              l_Entity.get_component(
                  Component::ConvexHullCollider::type_id());
          if (!l_Collider.is_alive() || l_Collider.is_trigger()) {
            return false;
          }

          if (!add_convex_hull_geometry(p_Geometry, l_Collider,
                                        l_Transform, l_AreaType)) {
            LOW_LOG_WARN << "Navigation source on entity '"
                         << l_Entity.get_name()
                         << "' has invalid ConvexHullCollider "
                            "geometry."
                         << LOW_LOG_END;
            return false;
          }
          return true;
        }

        static void
        debug_render_geometry(const BuildGeometry &p_Geometry,
                              const Math::Color &p_TriangleColor,
                              const Math::Color &p_BoundsColor,
                              const bool p_RenderBounds,
                              const bool p_Wireframe,
                              const bool p_UseAreaColors)
        {
          for (uint32_t i = 0u; i + 2u < p_Geometry.indices.size();
               i += 3u) {
            const uint32_t l_TriangleIndex = i / 3u;
            const uint32_t l_Index0 = p_Geometry.indices[i];
            const uint32_t l_Index1 = p_Geometry.indices[i + 1u];
            const uint32_t l_Index2 = p_Geometry.indices[i + 2u];

            if (l_Index0 >= p_Geometry.vertices.size() ||
                l_Index1 >= p_Geometry.vertices.size() ||
                l_Index2 >= p_Geometry.vertices.size()) {
              LOW_LOG_WARN
                  << "Navigation debug geometry index out of bounds."
                  << LOW_LOG_END;
              continue;
            }

            const Math::Color l_TriangleColor =
                p_UseAreaColors &&
                        l_TriangleIndex <
                            p_Geometry.triangle_area_types.size()
                    ? get_area_debug_color(
                          p_Geometry
                              .triangle_area_types[l_TriangleIndex])
                    : p_TriangleColor;

            DebugGeometry::render_triangle(
                p_Geometry.vertices[l_Index0],
                p_Geometry.vertices[l_Index1],
                p_Geometry.vertices[l_Index2], l_TriangleColor, false,
                p_Wireframe);
          }

          if (p_RenderBounds && !p_Geometry.vertices.empty()) {
            Math::Box l_Bounds;
            l_Bounds.position = (p_Geometry.bounds.min +
                                 p_Geometry.bounds.max) *
                                0.5f;
            l_Bounds.rotation =
                Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
            l_Bounds.halfExtents = (p_Geometry.bounds.max -
                                    p_Geometry.bounds.min) *
                                   0.5f;
            DebugGeometry::render_box(l_Bounds, p_BoundsColor, false,
                                      true);
          }
        }
      bool refresh_source_bounds(Source p_Source)
      {
        if (!p_Source.is_alive()) {
          return false;
        }

        Bounds l_Bounds;
        if (!compute_source_bounds(p_Source, &l_Bounds)) {
          p_Source.set_bounds_valid(false);
          return false;
        }

        p_Source.set_bounds(l_Bounds);
        p_Source.set_bounds_valid(true);
        return true;
      }

      bool get_source_bounds(Source p_Source, Bounds *p_Bounds)
      {
        if (!p_Bounds || !p_Source.is_alive()) {
          return false;
        }

        if (!p_Source.is_bounds_valid() &&
            !refresh_source_bounds(p_Source)) {
          return false;
        }

        *p_Bounds = p_Source.get_bounds();
        return true;
      }

      bool mark_source_dirty(Source p_Source)
      {
        if (!p_Source.is_alive()) {
          return false;
        }

        p_Source.set_tile_dirty(true);
        if (is_source_being_marked_dirty(p_Source)) {
          return false;
        }

        SourceDirtyGuard l_DirtyGuard(p_Source);

        Entity l_Entity = p_Source.get_entity();
        if (!l_Entity.is_alive()) {
          return false;
        }

        Region l_Region = l_Entity.get_region();
        if (!l_Region.is_alive()) {
          return false;
        }

        Scene l_Scene = l_Region.get_scene();
        if (!l_Scene.is_alive()) {
          return false;
        }

        World l_World = l_Scene.get_navigation_world();
        if (!l_World.is_alive()) {
          return false;
        }

        const BuildSettings l_Settings = get_build_settings(l_World);
        const float l_TileWorldSize = get_tile_world_size(l_Settings);
        bool l_DirtiedTile = false;

        auto dirty_bounds = [&](const Bounds &p_Bounds) {
          const TileRange l_Range =
              tile_range_for_bounds(p_Bounds, l_TileWorldSize);
          for (int x = l_Range.minimum.x; x <= l_Range.maximum.x;
               ++x) {
            for (int z = l_Range.minimum.z; z <= l_Range.maximum.z;
                 ++z) {
              const TileCoord i_Coord{x, z};
              ensure_tile(l_World, i_Coord, p_Bounds.min.y,
                          p_Bounds.max.y);
              set_tile_state(l_World, i_Coord, TileState::Dirty);
              l_DirtiedTile = true;
            }
          }
        };

        if (p_Source.is_bounds_valid()) {
          dirty_bounds(p_Source.get_bounds());
        }

        Bounds l_NewBounds;
        if (compute_source_bounds(p_Source, &l_NewBounds)) {
          dirty_bounds(l_NewBounds);
          p_Source.set_bounds(l_NewBounds);
          p_Source.set_bounds_valid(true);
        } else {
          p_Source.set_bounds_valid(false);
        }

        return l_DirtiedTile;
      }

      uint32_t update_invoker_tiles(World p_World, float p_MinY,
                                    float p_MaxY)
      {
        if (!p_World.is_alive()) {
          return 0u;
        }

        LOW_ASSERT(p_MinY <= p_MaxY,
                   "Navigation invoker tile Y range is invalid");

        const BuildSettings l_Settings = get_build_settings(p_World);
        const float l_TileWorldSize = get_tile_world_size(l_Settings);
        uint32_t l_DirtiedTileCount = 0u;
        WorldInvokerTileCache *l_Cache =
            get_world_invoker_tile_cache(p_World);
        begin_invoker_cache_update(l_Cache);

        for (uint32_t i = 0u; i < Invoker::living_count(); ++i) {
          Invoker i_Invoker = Invoker::living_instances()[i];
          if (!i_Invoker.is_alive() ||
              i_Invoker.get_generation_radius() < 0.0f) {
            continue;
          }

          Entity l_Entity = i_Invoker.get_entity();
          if (!l_Entity.is_alive()) {
            continue;
          }

          Region l_Region = l_Entity.get_region();
          if (!l_Region.is_alive()) {
            continue;
          }

          Scene l_Scene = l_Region.get_scene();
          if (!l_Scene.is_alive() ||
              l_Scene.get_navigation_world().get_id() !=
                  p_World.get_id()) {
            continue;
          }

          Component::Transform l_Transform =
              l_Entity.get_transform();
          if (!l_Transform.is_alive()) {
            continue;
          }

          const TileRange l_GenerationRange = tile_range_for_radius(
              l_Transform.get_world_position(),
              i_Invoker.get_generation_radius(), l_TileWorldSize);
          const float l_RemovalRadius =
              std::max(0.0f, i_Invoker.get_removal_radius());
          const TileRange l_RemovalRange = tile_range_for_radius(
              l_Transform.get_world_position(),
              l_RemovalRadius, l_TileWorldSize);

          InvokerTileCacheEntry *l_CacheEntry =
              find_invoker_cache_entry(
                  l_Cache, i_Invoker.get_unique_id());
          bool l_GenerationChanged = true;
          bool l_RemovalChanged = true;
          if (l_CacheEntry) {
            l_GenerationChanged =
                !tile_ranges_equal(l_CacheEntry->generation_range,
                                   l_GenerationRange) ||
                l_CacheEntry->generation_radius !=
                    i_Invoker.get_generation_radius();
            l_RemovalChanged =
                !tile_ranges_equal(l_CacheEntry->removal_range,
                                   l_RemovalRange) ||
                l_CacheEntry->removal_radius != l_RemovalRadius;
          } else {
            InvokerTileCacheEntry l_NewEntry;
            l_NewEntry.invoker_id = i_Invoker.get_unique_id();
            l_Cache->entries.push_back(l_NewEntry);
            l_CacheEntry = &l_Cache->entries.back();
          }

          l_CacheEntry->generation_range = l_GenerationRange;
          l_CacheEntry->removal_range = l_RemovalRange;
          l_CacheEntry->generation_radius =
              i_Invoker.get_generation_radius();
          l_CacheEntry->removal_radius = l_RemovalRadius;
          l_CacheEntry->touched = true;

          if (l_RemovalChanged) {
            l_Cache->eviction_requested = true;
          }

          if (!l_GenerationChanged) {
            continue;
          }

          for (int x = l_GenerationRange.minimum.x;
               x <= l_GenerationRange.maximum.x;
               ++x) {
            for (int z = l_GenerationRange.minimum.z;
                 z <= l_GenerationRange.maximum.z;
                 ++z) {
              const TileCoord i_Coord{x, z};
              Tile i_Tile;
              const bool l_HadTile =
                  get_tile(p_World, i_Coord, &i_Tile);

              ensure_tile(p_World, i_Coord, p_MinY, p_MaxY,
                          &i_Tile);
              if (!l_HadTile || i_Tile.state == TileState::Empty) {
                set_tile_state(p_World, i_Coord,
                               TileState::Dirty);
                ++l_DirtiedTileCount;
              }
            }
          }
        }

        end_invoker_cache_update(l_Cache);
        return l_DirtiedTileCount;
      }

      uint32_t evict_tiles_outside_invokers(
          World p_World, uint32_t p_MaxTilesToEvict)
      {
        if (!p_World.is_alive()) {
          return 0u;
        }

        Low::Util::List<InvokerInfo> l_Invokers;
        collect_world_invokers(p_World, &l_Invokers);

        Low::Util::List<Tile> l_Tiles;
        collect_tiles(p_World, &l_Tiles);

        uint32_t l_RemovedCount = 0u;
        for (const Tile &i_Tile : l_Tiles) {
          if (p_MaxTilesToEvict > 0u &&
              l_RemovedCount >= p_MaxTilesToEvict) {
            break;
          }

          if (tile_inside_any_invoker_removal_radius(i_Tile,
                                                     l_Invokers)) {
            continue;
          }

          if (remove_tile(p_World, i_Tile.coord)) {
            ++l_RemovedCount;
          }
        }
        return l_RemovedCount;
      }

      bool should_run_eviction(World p_World,
                               uint32_t p_EvictionIntervalTicks)
      {
        if (!p_World.is_alive()) {
          return false;
        }

        WorldInvokerTileCache *l_Cache =
            get_world_invoker_tile_cache(p_World);
        ++l_Cache->ticks_since_eviction;

        if (l_Cache->eviction_requested) {
          l_Cache->eviction_requested = false;
          l_Cache->ticks_since_eviction = 0u;
          return true;
        }

        if (p_EvictionIntervalTicks > 0u &&
            l_Cache->ticks_since_eviction >=
                p_EvictionIntervalTicks) {
          l_Cache->ticks_since_eviction = 0u;
          return true;
        }

        return false;
      }

      uint32_t queue_dirty_tiles(World p_World,
                                 uint32_t p_MaxTilesToQueue)
      {
        if (!p_World.is_alive()) {
          return 0u;
        }

        Low::Util::List<InvokerInfo> l_Invokers;
        if (!collect_world_invokers(p_World, &l_Invokers)) {
          return 0u;
        }

        Low::Util::List<Tile> l_Tiles;
        collect_dirty_tiles(p_World, &l_Tiles);

        struct QueueCandidate
        {
          TileCoord coord;
          float priority = 0.0f;
        };

        Low::Util::List<QueueCandidate> l_Candidates;
        for (const Tile &i_Tile : l_Tiles) {
          if (i_Tile.state != TileState::Dirty) {
            continue;
          }

          QueueCandidate l_Candidate;
          l_Candidate.coord = i_Tile.coord;
          l_Candidate.priority =
              nearest_invoker_distance_squared(i_Tile, l_Invokers);
          l_Candidates.push_back(l_Candidate);
        }

        std::sort(l_Candidates.begin(), l_Candidates.end(),
                  [](const QueueCandidate &p_A,
                     const QueueCandidate &p_B) {
                    return p_A.priority < p_B.priority;
                  });

        uint32_t l_QueuedCount = 0u;
        for (const QueueCandidate &i_Candidate : l_Candidates) {
          if (p_MaxTilesToQueue > 0u &&
              l_QueuedCount >= p_MaxTilesToQueue) {
            break;
          }

          if (queue_tile(p_World, i_Candidate.coord)) {
            ++l_QueuedCount;
          }
        }

        return l_QueuedCount;
      }

      bool collect_build_geometry(BuildGeometry *p_Geometry)
      {
        if (!p_Geometry) {
          return false;
        }

        p_Geometry->vertices.clear();
        p_Geometry->indices.clear();
        p_Geometry->triangle_area_types.clear();
        p_Geometry->bounds = Bounds();

        for (uint32_t i = 0u; i < Source::living_count(); ++i) {
          append_source_geometry(*p_Geometry,
                                 Source::living_instances()[i]);
        }

        recompute_bounds(*p_Geometry);

        return !p_Geometry->vertices.empty() &&
               !p_Geometry->indices.empty();
      }

      bool collect_build_geometry_for_bounds(
          const Bounds &p_Bounds, BuildGeometry *p_Geometry)
      {
        return collect_build_geometry_for_world_bounds(
            World(), p_Bounds, p_Geometry);
      }

      bool collect_build_geometry_for_world_bounds(
          World p_World, const Bounds &p_Bounds,
          BuildGeometry *p_Geometry)
      {
        if (!p_Geometry) {
          return false;
        }

        p_Geometry->vertices.clear();
        p_Geometry->indices.clear();
        p_Geometry->triangle_area_types.clear();
        p_Geometry->bounds = p_Bounds;

        for (uint32_t i = 0u; i < Source::living_count(); ++i) {
          Source i_Source = Source::living_instances()[i];
          if (p_World.is_alive() &&
              !source_belongs_to_world(i_Source, p_World)) {
            continue;
          }

          Bounds i_SourceBounds;
          if (!get_source_bounds(i_Source, &i_SourceBounds) ||
              !bounds_overlap(p_Bounds, i_SourceBounds)) {
            continue;
          }

          append_source_geometry(*p_Geometry, i_Source);
        }

        if (p_Geometry->vertices.empty() ||
            p_Geometry->indices.empty()) {
          return false;
        }

        p_Geometry->bounds = p_Bounds;
        return true;
      }

      bool build_tile(World p_World, TileCoord p_Coord)
      {
        if (!p_World.is_alive()) {
          return false;
        }

        Tile l_Tile;
        if (!get_tile(p_World, p_Coord, &l_Tile)) {
          return false;
        }

        const BuildSettings l_Settings = get_build_settings(p_World);
        const int l_WalkableRadius = static_cast<int>(std::ceil(
            l_Settings.agent_radius / l_Settings.cell_size));
        const float l_BorderPadding =
            static_cast<float>(l_WalkableRadius + 3) *
            l_Settings.cell_size;
        const Bounds l_BuildBounds =
            expand_bounds(l_Tile.bounds, l_BorderPadding);

        BuildGeometry l_Geometry;
        if (!collect_build_geometry_for_world_bounds(
                p_World, l_BuildBounds, &l_Geometry)) {
          set_tile_state(p_World, p_Coord, TileState::Failed);
          return false;
        }

        return build_tile_from_geometry(p_World, p_Coord,
                                        l_Geometry);
      }

      void
      debug_render_build_geometry(const BuildGeometry &p_Geometry)
      {
        const Math::Color l_TriangleColor(0.0f, 0.6f, 1.0f, 0.2f);
        const Math::Color l_BoundsColor(0.0f, 0.8f, 1.0f, 1.0f);

        debug_render_geometry(p_Geometry, l_TriangleColor,
                              l_BoundsColor, true, true, false);
      }

      bool is_build_geometry_debug_rendering_enabled()
      {
        return g_BuildGeometryDebugRenderingEnabled;
      }

      void set_build_geometry_debug_rendering_enabled(bool p_Enabled)
      {
        g_BuildGeometryDebugRenderingEnabled = p_Enabled;
      }

      void
      debug_render_navmesh_geometry(const BuildGeometry &p_Geometry)
      {
        const Math::Color l_TriangleColor(1.0f, 0.65f, 0.05f, 0.18f);
        const Math::Color l_BoundsColor(1.0f, 0.65f, 0.05f, 1.0f);

        debug_render_geometry(p_Geometry, l_TriangleColor,
                              l_BoundsColor, false, false, true);
      }

      bool is_navmesh_debug_rendering_enabled()
      {
        return g_NavmeshDebugRenderingEnabled;
      }

      void set_navmesh_debug_rendering_enabled(bool p_Enabled)
      {
        g_NavmeshDebugRenderingEnabled = p_Enabled;
      }

      void debug_render_tile_registry(World p_World)
      {
        if (!p_World.is_alive()) {
          return;
        }

        Low::Util::List<Tile> l_Tiles;
        collect_tiles(p_World, &l_Tiles);

        for (const Tile &i_Tile : l_Tiles) {
          const Math::Vector3 l_Center =
              (i_Tile.bounds.min + i_Tile.bounds.max) * 0.5f;
          Math::Box l_Box;
          l_Box.position =
              Math::Vector3(l_Center.x, 0.05f, l_Center.z);
          l_Box.rotation =
              Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
          l_Box.halfExtents = Math::Vector3(
              (i_Tile.bounds.max.x - i_Tile.bounds.min.x) * 0.5f,
              0.05f,
              (i_Tile.bounds.max.z - i_Tile.bounds.min.z) * 0.5f);

          DebugGeometry::render_box(
              l_Box, get_tile_debug_color(i_Tile.state), false,
              true);
        }
      }

      bool is_tile_debug_rendering_enabled()
      {
        return g_TileDebugRenderingEnabled;
      }

      void set_tile_debug_rendering_enabled(bool p_Enabled)
      {
        g_TileDebugRenderingEnabled = p_Enabled;
      }

      void debug_render_invokers(World p_World)
      {
        if (!p_World.is_alive()) {
          return;
        }

        Low::Util::List<InvokerInfo> l_Invokers;
        if (!collect_world_invokers(p_World, &l_Invokers)) {
          return;
        }

        for (const InvokerInfo &i_Invoker : l_Invokers) {
          Math::Cylinder l_Generation;
          l_Generation.position =
              i_Invoker.position + Math::Vector3(0.0f, 0.08f, 0.0f);
          l_Generation.rotation =
              Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
          l_Generation.radius =
              std::max(0.0f, i_Invoker.generation_radius);
          l_Generation.height = 0.08f;

          Math::Cylinder l_Removal = l_Generation;
          l_Removal.position =
              i_Invoker.position + Math::Vector3(0.0f, 0.16f, 0.0f);
          l_Removal.radius =
              std::max(0.0f, i_Invoker.removal_radius);

          DebugGeometry::render_cylinder(
              l_Generation, Math::Color(0.15f, 0.75f, 1.0f, 0.5f),
              false, true);
          DebugGeometry::render_cylinder(
              l_Removal, Math::Color(1.0f, 0.55f, 0.15f, 0.45f),
              false, true);
        }
      }

      bool is_invoker_debug_rendering_enabled()
      {
        return g_InvokerDebugRenderingEnabled;
      }

      void set_invoker_debug_rendering_enabled(bool p_Enabled)
      {
        g_InvokerDebugRenderingEnabled = p_Enabled;
      }

      void render_debug_geometry()
      {
        Scene l_Scene = Scene::get_loaded_scene();

        if (g_BuildGeometryDebugRenderingEnabled) {
          BuildGeometry l_Geometry;
          if (collect_build_geometry(&l_Geometry)) {
            debug_render_build_geometry(l_Geometry);
          }
        }

        if (g_NavmeshDebugRenderingEnabled) {
          if (!l_Scene.is_alive()) {
            return;
          }

          BuildGeometry l_Geometry;
          if (collect_navmesh_geometry(l_Scene.get_navigation_world(),
                                       &l_Geometry)) {
            debug_render_navmesh_geometry(l_Geometry);
          }
        }

        if (l_Scene.is_alive()) {
          World l_World = l_Scene.get_navigation_world();
          if (l_World.is_alive()) {
            if (g_TileDebugRenderingEnabled) {
              debug_render_tile_registry(l_World);
            }

            if (g_InvokerDebugRenderingEnabled) {
              debug_render_invokers(l_World);
            }
          }
        }
      }

      bool
      collect_build_geometry_from_scene(Scene p_Scene,
                                        BuildGeometry *p_Geometry)
      {
        (void)p_Scene;
        return collect_build_geometry(p_Geometry);
      }
    } // namespace Navigation
  } // namespace Core
} // namespace Low
