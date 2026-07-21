#include "LowCoreNavigation.h"

#include "LowCoreBoxCollider.h"
#include "LowCoreCharacterController.h"
#include "LowCoreConvexHullCollider.h"
#include "LowCoreDebugGeometry.h"
#include "LowCoreEntity.h"
#include "LowCoreNavigationSource.h"
#include "LowCoreNavigationWorld.h"
#include "LowCoreScene.h"
#include "LowCoreSphereCollider.h"
#include "LowCoreTransform.h"

#include "LowUtilLogger.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Low {
  namespace Core {
    namespace Navigation {
      static bool g_BuildGeometryDebugRenderingEnabled = false;
      static bool g_NavmeshDebugRenderingEnabled = false;

      namespace {
        static void include_point(BuildGeometry &p_Geometry,
                                  const Math::Vector3 &p_Point)
        {
          if (p_Geometry.vertices.size() == 1u) {
            p_Geometry.bounds.minimum = p_Point;
            p_Geometry.bounds.maximum = p_Point;
            return;
          }

          p_Geometry.bounds.minimum.x =
              std::min(p_Geometry.bounds.minimum.x, p_Point.x);
          p_Geometry.bounds.minimum.y =
              std::min(p_Geometry.bounds.minimum.y, p_Point.y);
          p_Geometry.bounds.minimum.z =
              std::min(p_Geometry.bounds.minimum.z, p_Point.z);
          p_Geometry.bounds.maximum.x =
              std::max(p_Geometry.bounds.maximum.x, p_Point.x);
          p_Geometry.bounds.maximum.y =
              std::max(p_Geometry.bounds.maximum.y, p_Point.y);
          p_Geometry.bounds.maximum.z =
              std::max(p_Geometry.bounds.maximum.z, p_Point.z);
        }

        static void recompute_bounds(BuildGeometry &p_Geometry)
        {
          p_Geometry.bounds = Bounds();
          for (uint32_t i = 0u; i < p_Geometry.vertices.size(); ++i) {
            if (i == 0u) {
              p_Geometry.bounds.minimum = p_Geometry.vertices[i];
              p_Geometry.bounds.maximum = p_Geometry.vertices[i];
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

        static bool append_source_geometry(BuildGeometry &p_Geometry,
                                           Source p_Source)
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
            LOW_LOG_WARN << "Navigation source on entity '"
                         << l_Entity.get_name()
                         << "' uses unsupported modifier mode."
                         << LOW_LOG_END;
            return false;
          }

          if (p_Source.get_geometry_type() !=
              SourceGeometryType::COLLIDER) {
            LOW_LOG_WARN << "Navigation source on entity '"
                         << l_Entity.get_name()
                         << "' uses unsupported geometry type."
                         << LOW_LOG_END;
            return false;
          }

          if (!l_Entity.has_component(
                  Component::BoxCollider::type_id()) &&
              !l_Entity.has_component(
                  Component::SphereCollider::type_id()) &&
              !l_Entity.has_component(
                  Component::ConvexHullCollider::type_id())) {
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
            return false;
          }

          Component::Transform l_Transform = l_Entity.get_transform();
          if (!l_Transform.is_alive()) {
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
            l_Bounds.position = (p_Geometry.bounds.minimum +
                                 p_Geometry.bounds.maximum) *
                                0.5f;
            l_Bounds.rotation =
                Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
            l_Bounds.halfExtents = (p_Geometry.bounds.maximum -
                                    p_Geometry.bounds.minimum) *
                                   0.5f;
            DebugGeometry::render_box(l_Bounds, p_BoundsColor, false,
                                      true);
          }
        }
      } // namespace

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

      void render_debug_geometry()
      {
        if (g_BuildGeometryDebugRenderingEnabled) {
          BuildGeometry l_Geometry;
          if (collect_build_geometry(&l_Geometry)) {
            debug_render_build_geometry(l_Geometry);
          }
        }

        if (g_NavmeshDebugRenderingEnabled) {
          Scene l_Scene = Scene::get_loaded_scene();
          if (!l_Scene.is_alive()) {
            return;
          }

          BuildGeometry l_Geometry;
          if (collect_navmesh_geometry(l_Scene.get_navigation_world(),
                                       &l_Geometry)) {
            debug_render_navmesh_geometry(l_Geometry);
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
