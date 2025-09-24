#include "LowRendererPrimitives.h"

#include "LowUtilResource.h"

#include "LowRendererResourceManager.h"

namespace Low {
  namespace Renderer {
    Primitives g_Primitives;

    void initialize_primitives()
    {
      g_Primitives.unitQuad = create_quad({1.0f, 1.0f});
      g_Primitives.unitQuad.set_unloadable(true);
      ResourceManager::load_mesh(g_Primitives.unitQuad);

      g_Primitives.unitCube = create_cube({1.0f, 1.0f, 1.0f});
      g_Primitives.unitCube.set_unloadable(true);
      ResourceManager::load_mesh(g_Primitives.unitCube);

      g_Primitives.unitIcoSphere = create_icosphere(1.0f, 1);
      g_Primitives.unitIcoSphere.set_unloadable(true);
      ResourceManager::load_mesh(g_Primitives.unitIcoSphere);
    }

    Primitives &get_primitives()
    {
      return g_Primitives;
    }

    static Mesh
    create_mesh(const Util::Name p_Name,
                Util::List<Util::Resource::Vertex> &p_Vertices,
                Util::List<u32> &p_Indices)
    {
      Mesh l_Mesh = Mesh::make(p_Name);
      MeshGeometry l_Geometry = MeshGeometry::make(p_Name);

      SubmeshGeometry l_SubmeshGeometry =
          SubmeshGeometry::make(p_Name);
      l_SubmeshGeometry.set_vertices(p_Vertices);
      l_SubmeshGeometry.set_vertex_count(p_Vertices.size());

      l_SubmeshGeometry.set_indices(p_Indices);
      l_SubmeshGeometry.set_index_count(p_Indices.size());

      l_SubmeshGeometry.set_index_count(
          l_SubmeshGeometry.get_indices().size());

      l_SubmeshGeometry.set_state(MeshState::MEMORYLOADED);

      // TODO: Calculate AABB and bounding sphere

      l_Geometry.get_submeshes().push_back(l_SubmeshGeometry);
      l_Geometry.set_submesh_count(1);

      l_Mesh.set_geometry(l_Geometry);
      l_Mesh.set_state(MeshState::MEMORYLOADED);

      return l_Mesh;
    }

    Mesh create_quad(Math::Vector2 p_Size)
    {
      const Math::Vector3 l_Normal(0.0f, 0.0f, 1.0f);
      const Math::Vector3 l_Tangent(1.0f, 0.0f, 0.0f);
      const Math::Vector3 l_Bitangent(0.0f, 1.0f, 0.0f);

      const float l_HalfWidth = p_Size.x * 0.5f;
      const float l_HalfHeight = p_Size.y * 0.5f;

      Low::Util::List<Low::Util::Resource::Vertex> l_Vertices;

#if 1
      l_Vertices.push_back(
          {Math::Vector3(-l_HalfWidth, -l_HalfHeight, 0.0f),
           Math::Vector2(0.0f, 1.0f), l_Normal, l_Tangent,
           l_Bitangent});
      l_Vertices.push_back(
          {Math::Vector3(l_HalfWidth, -l_HalfHeight, 0.0f),
           Math::Vector2(1.0f, 1.0f), l_Normal, l_Tangent,
           l_Bitangent});
      l_Vertices.push_back(
          {Math::Vector3(l_HalfWidth, l_HalfHeight, 0.0f),
           Math::Vector2(1.0f, 0.0f), l_Normal, l_Tangent,
           l_Bitangent});
      l_Vertices.push_back(
          {Math::Vector3(-l_HalfWidth, l_HalfHeight, 0.0f),
           Math::Vector2(0.0f, 0.0f), l_Normal, l_Tangent,
           l_Bitangent});
#else
      l_Vertices.push_back(
          {Math::Vector3(-l_HalfWidth, -l_HalfHeight, 0.0f),
           Math::Vector2(0.0f, 0.0f), l_Normal, l_Tangent,
           l_Bitangent});
      l_Vertices.push_back(
          {Math::Vector3(l_HalfWidth, -l_HalfHeight, 0.0f),
           Math::Vector2(1.0f, 0.0f), l_Normal, l_Tangent,
           l_Bitangent});
      l_Vertices.push_back(
          {Math::Vector3(l_HalfWidth, l_HalfHeight, 0.0f),
           Math::Vector2(1.0f, 1.0f), l_Normal, l_Tangent,
           l_Bitangent});
      l_Vertices.push_back(
          {Math::Vector3(-l_HalfWidth, l_HalfHeight, 0.0f),
           Math::Vector2(0.0f, 1.0f), l_Normal, l_Tangent,
           l_Bitangent});
#endif

      Util::List<u32> l_Indices;
      l_Indices.push_back(0);
      l_Indices.push_back(1);
      l_Indices.push_back(2);

      l_Indices.push_back(2);
      l_Indices.push_back(3);
      l_Indices.push_back(0);

      return create_mesh(N(PrimitiveUnitQuad), l_Vertices, l_Indices);
    }

    Mesh create_cube(Math::Vector3 p_HalfExtents)
    {
      Util::List<Util::Resource::Vertex> l_Vertices;
      Util::List<u32> l_Indices;
      l_Vertices.reserve(24);
      l_Indices.reserve(36);

      // Each face: normal, tangent, bitangent
      struct FaceDesc
      {
        Math::Vector3 normal;
        Math::Vector3 tangent;
        Math::Vector3 bitangent;
        Math::Vector3 corners[4];
      };

      FaceDesc faces[6] = {
          // Front (+Z)
          {Math::Vector3(0, 0, 1),
           Math::Vector3(1, 0, 0),
           Math::Vector3(0, 1, 0),
           {Math::Vector3(-p_HalfExtents.x, -p_HalfExtents.y,
                          p_HalfExtents.z),
            Math::Vector3(p_HalfExtents.x, -p_HalfExtents.y,
                          p_HalfExtents.z),
            Math::Vector3(p_HalfExtents.x, p_HalfExtents.y,
                          p_HalfExtents.z),
            Math::Vector3(-p_HalfExtents.x, p_HalfExtents.y,
                          p_HalfExtents.z)}},

          // Back (-Z)
          {Math::Vector3(0, 0, -1),
           Math::Vector3(-1, 0, 0),
           Math::Vector3(0, 1, 0),
           {Math::Vector3(p_HalfExtents.x, -p_HalfExtents.y,
                          -p_HalfExtents.z),
            Math::Vector3(-p_HalfExtents.x, -p_HalfExtents.y,
                          -p_HalfExtents.z),
            Math::Vector3(-p_HalfExtents.x, p_HalfExtents.y,
                          -p_HalfExtents.z),
            Math::Vector3(p_HalfExtents.x, p_HalfExtents.y,
                          -p_HalfExtents.z)}},

          // Right (+X)
          {Math::Vector3(1, 0, 0),
           Math::Vector3(0, 0, -1),
           Math::Vector3(0, 1, 0),
           {Math::Vector3(p_HalfExtents.x, -p_HalfExtents.y,
                          p_HalfExtents.z),
            Math::Vector3(p_HalfExtents.x, -p_HalfExtents.y,
                          -p_HalfExtents.z),
            Math::Vector3(p_HalfExtents.x, p_HalfExtents.y,
                          -p_HalfExtents.z),
            Math::Vector3(p_HalfExtents.x, p_HalfExtents.y,
                          p_HalfExtents.z)}},

          // Left (-X)
          {Math::Vector3(-1, 0, 0),
           Math::Vector3(0, 0, 1),
           Math::Vector3(0, 1, 0),
           {Math::Vector3(-p_HalfExtents.x, -p_HalfExtents.y,
                          -p_HalfExtents.z),
            Math::Vector3(-p_HalfExtents.x, -p_HalfExtents.y,
                          p_HalfExtents.z),
            Math::Vector3(-p_HalfExtents.x, p_HalfExtents.y,
                          p_HalfExtents.z),
            Math::Vector3(-p_HalfExtents.x, p_HalfExtents.y,
                          -p_HalfExtents.z)}},

          // Top (+Y)
          {Math::Vector3(0, 1, 0),
           Math::Vector3(1, 0, 0),
           Math::Vector3(0, 0, -1),
           {Math::Vector3(-p_HalfExtents.x, p_HalfExtents.y,
                          p_HalfExtents.z),
            Math::Vector3(p_HalfExtents.x, p_HalfExtents.y,
                          p_HalfExtents.z),
            Math::Vector3(p_HalfExtents.x, p_HalfExtents.y,
                          -p_HalfExtents.z),
            Math::Vector3(-p_HalfExtents.x, p_HalfExtents.y,
                          -p_HalfExtents.z)}},

          // Bottom (-Y)
          {Math::Vector3(0, -1, 0),
           Math::Vector3(1, 0, 0),
           Math::Vector3(0, 0, 1),
           {Math::Vector3(-p_HalfExtents.x, -p_HalfExtents.y,
                          -p_HalfExtents.z),
            Math::Vector3(p_HalfExtents.x, -p_HalfExtents.y,
                          -p_HalfExtents.z),
            Math::Vector3(p_HalfExtents.x, -p_HalfExtents.y,
                          p_HalfExtents.z),
            Math::Vector3(-p_HalfExtents.x, -p_HalfExtents.y,
                          p_HalfExtents.z)}}};

      // Fill vertices + indices
      for (int f = 0; f < 6; ++f) {
        u32 baseIndex = (u32)l_Vertices.size();

        l_Vertices.push_back({faces[f].corners[0],
                              Math::Vector2(0, 0), faces[f].normal,
                              faces[f].tangent, faces[f].bitangent});
        l_Vertices.push_back({faces[f].corners[1],
                              Math::Vector2(1, 0), faces[f].normal,
                              faces[f].tangent, faces[f].bitangent});
        l_Vertices.push_back({faces[f].corners[2],
                              Math::Vector2(1, 1), faces[f].normal,
                              faces[f].tangent, faces[f].bitangent});
        l_Vertices.push_back({faces[f].corners[3],
                              Math::Vector2(0, 1), faces[f].normal,
                              faces[f].tangent, faces[f].bitangent});

        l_Indices.push_back(baseIndex + 0);
        l_Indices.push_back(baseIndex + 1);
        l_Indices.push_back(baseIndex + 2);

        l_Indices.push_back(baseIndex + 2);
        l_Indices.push_back(baseIndex + 3);
        l_Indices.push_back(baseIndex + 0);
      }

      return create_mesh(N(cube), l_Vertices, l_Indices);
    }

    namespace {
      struct EdgeKey
      {
        u32 a;
        u32 b;

        EdgeKey(u32 p_A, u32 p_B)
        {
          if (p_A < p_B) {
            a = p_A;
            b = p_B;
          } else {
            a = p_B;
            b = p_A;
          }
        }

        // eastl::map uses < comparator
        bool operator<(const EdgeKey &p_Other) const
        {
          if (a != p_Other.a)
            return a < p_Other.a;
          return b < p_Other.b;
        }
      };

      inline Util::Resource::Vertex make_vertex_from_unit_normal(
          const Low::Math::Vector3 &p_UnitNormal, float p_Radius)
      {
        using namespace Low::Util::Resource;
        using namespace Low::Math;

        Vector3 l_Normal = glm::normalize(p_UnitNormal);
        Vector3 l_Position = l_Normal * p_Radius;

        // Spherical UV mapping (seam at +/-PI on atan2)
        // u in [0,1], v in [0,1]
        float l_U = (glm::atan(l_Normal.z, l_Normal.x) /
                     (2.0f * glm::pi<float>())) +
                    0.5f;
        float l_V = (glm::asin(glm::clamp(l_Normal.y, -1.0f, 1.0f)) /
                     glm::pi<float>()) +
                    0.5f;

        // Tangent basis:
        // Choose a stable "up" that is not parallel to normal
        Vector3 l_Up(0.0f, 1.0f, 0.0f);
        if (glm::abs(glm::dot(l_Up, l_Normal)) > 0.99f) {
          l_Up = Vector3(1.0f, 0.0f, 0.0f);
        }

        // Tangent points in direction of increasing U approximately
        Vector3 l_Tangent =
            glm::normalize(glm::cross(l_Up, l_Normal));
        Vector3 l_Bitangent =
            glm::normalize(glm::cross(l_Normal, l_Tangent));

        Vertex l_Vert{};
        l_Vert.position = l_Position;
        l_Vert.texture_coordinates = Low::Math::Vector2(l_U, l_V);
        l_Vert.normal = l_Normal;
        l_Vert.tangent = l_Tangent;
        l_Vert.bitangent = l_Bitangent;
        return l_Vert;
      }

      inline void add_initial_icosahedron(
          Low::Util::List<Low::Math::Vector3> &p_Points,
          Low::Util::List<u32> &p_Indices)
      {
        using namespace Low::Math;

        const float l_Phi =
            (1.0f + glm::sqrt(5.0f)) * 0.5f; // golden ratio
        const float l_InvLen = 1.0f / glm::sqrt(1.0f + l_Phi * l_Phi);

        // Build the 12 vertices of a canonical icosahedron,
        // normalized to unit length (±1, ±phi, 0), (0, ±1, ±phi),
        // (±phi, 0, ±1)
        Vector3 l_V[12] = {
            glm::normalize(Vector3(-1.0f, l_Phi, 0.0f)),
            glm::normalize(Vector3(1.0f, l_Phi, 0.0f)),
            glm::normalize(Vector3(-1.0f, -l_Phi, 0.0f)),
            glm::normalize(Vector3(1.0f, -l_Phi, 0.0f)),

            glm::normalize(Vector3(0.0f, -1.0f, l_Phi)),
            glm::normalize(Vector3(0.0f, 1.0f, l_Phi)),
            glm::normalize(Vector3(0.0f, -1.0f, -l_Phi)),
            glm::normalize(Vector3(0.0f, 1.0f, -l_Phi)),

            glm::normalize(Vector3(l_Phi, 0.0f, -1.0f)),
            glm::normalize(Vector3(l_Phi, 0.0f, 1.0f)),
            glm::normalize(Vector3(-l_Phi, 0.0f, -1.0f)),
            glm::normalize(Vector3(-l_Phi, 0.0f, 1.0f))};

        p_Points.reserve(p_Points.size() + 12);
        for (int i_Idx = 0; i_Idx < 12; ++i_Idx) {
          p_Points.push_back(l_V[i_Idx]);
        }

        // 20 faces (CCW)
        static const u32 l_Tris[60] = {
            0, 11, 5, 0, 5,  1,  0,  1,  7,  0,  7, 10, 0, 10, 11,
            1, 5,  9, 5, 11, 4,  11, 10, 2,  10, 7, 6,  7, 1,  8,
            3, 9,  4, 3, 4,  2,  3,  2,  6,  3,  6, 8,  3, 8,  9,
            4, 9,  5, 2, 4,  11, 6,  2,  10, 8,  6, 7,  9, 8,  1};

        p_Indices.reserve(p_Indices.size() + 60);
        for (int i_Idx = 0; i_Idx < 60; ++i_Idx) {
          p_Indices.push_back(l_Tris[i_Idx]);
        }
      }

      inline u32
      midpoint_index(u32 p_I0, u32 p_I1,
                     Low::Util::List<Low::Math::Vector3> &p_Points,
                     Low::Util::Map<EdgeKey, u32> &p_Cache)
      {
        using namespace Low::Math;

        EdgeKey l_Key(p_I0, p_I1);
        auto l_It = p_Cache.find(l_Key);
        if (l_It != p_Cache.end()) {
          return l_It->second;
        }

        Vector3 l_Mid =
            glm::normalize((p_Points[p_I0] + p_Points[p_I1]) * 0.5f);
        u32 l_NewIndex = static_cast<u32>(p_Points.size());
        p_Points.push_back(l_Mid);
        p_Cache[l_Key] = l_NewIndex;
        return l_NewIndex;
      }
    } // anonymous namespace

    Mesh create_icosphere(float p_Radius, u32 p_Subdivisions)
    {
      Util::List<Math::Vector3> l_UnitPoints;
      Util::List<u32> l_Indices;

      // Seed with base icosahedron
      add_initial_icosahedron(l_UnitPoints, l_Indices);

      // Subdivide
      for (u32 i_L = 0; i_L < p_Subdivisions; ++i_L) {
        Util::List<u32> l_NewIndices;
        l_NewIndices.reserve(l_Indices.size() *
                             4); // each tri -> 4 tris

        Util::Map<EdgeKey, u32> l_MidpointCache;

        for (size_t i_T = 0; i_T < l_Indices.size(); i_T += 3) {
          u32 l_I0 = l_Indices[i_T + 0];
          u32 l_I1 = l_Indices[i_T + 1];
          u32 l_I2 = l_Indices[i_T + 2];

          u32 l_A = midpoint_index(l_I0, l_I1, l_UnitPoints,
                                   l_MidpointCache);
          u32 l_B = midpoint_index(l_I1, l_I2, l_UnitPoints,
                                   l_MidpointCache);
          u32 l_C = midpoint_index(l_I2, l_I0, l_UnitPoints,
                                   l_MidpointCache);

          // 4 new triangles (CCW)
          l_NewIndices.push_back(l_I0);
          l_NewIndices.push_back(l_A);
          l_NewIndices.push_back(l_C);
          l_NewIndices.push_back(l_I1);
          l_NewIndices.push_back(l_B);
          l_NewIndices.push_back(l_A);
          l_NewIndices.push_back(l_I2);
          l_NewIndices.push_back(l_C);
          l_NewIndices.push_back(l_B);
          l_NewIndices.push_back(l_A);
          l_NewIndices.push_back(l_B);
          l_NewIndices.push_back(l_C);
        }

        l_Indices.swap(l_NewIndices);
      }

      Util::List<Util::Resource::Vertex> l_Vertices;
      l_Vertices.reserve(l_UnitPoints.size());

      for (size_t i_V = 0; i_V < l_UnitPoints.size(); ++i_V) {
        l_Vertices.push_back(make_vertex_from_unit_normal(
            l_UnitPoints[i_V], p_Radius));
      }

      return create_mesh(N(IcoSphere), l_Vertices, l_Indices);
    }
  } // namespace Renderer
} // namespace Low
