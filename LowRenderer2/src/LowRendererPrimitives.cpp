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

      Util::List<u32> l_Indices;
      l_Indices.push_back(0);
      l_Indices.push_back(1);
      l_Indices.push_back(2);

      l_Indices.push_back(2);
      l_Indices.push_back(3);
      l_Indices.push_back(0);

      return create_mesh(N(PrimitiveUnitQuad), l_Vertices, l_Indices);
    }
  } // namespace Renderer
} // namespace Low
