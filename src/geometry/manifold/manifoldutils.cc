// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.

#include "manifoldutils.h"
#include "ManifoldGeometry.h"
#include "manifold.h"
#include "IndexedMesh.h"
#include "printutils.h"
#include "cgalutils.h"
#include "PolySetUtils.h"
#include "CGALHybridPolyhedron.h"
#include <CGAL/convex_hull_3.h>

// using namespace manifold;

namespace ManifoldUtils {

const char* statusToString(manifold::Manifold::Error status) {
  switch (status) {
    case manifold::Manifold::Error::NoError: return "NoError";
    case manifold::Manifold::Error::NonFiniteVertex: return "NonFiniteVertex";
    case manifold::Manifold::Error::NotManifold: return "NotManifold";
    case manifold::Manifold::Error::VertexOutOfBounds: return "VertexOutOfBounds";
    case manifold::Manifold::Error::PropertiesWrongLength: return "PropertiesWrongLength";
    case manifold::Manifold::Error::MissingPositionProperties: return "MissingPositionProperties";
    case manifold::Manifold::Error::MergeVectorsDifferentLengths: return "MergeVectorsDifferentLengths";
    case manifold::Manifold::Error::MergeIndexOutOfBounds: return "MergeIndexOutOfBounds";
    case manifold::Manifold::Error::TransformWrongLength: return "TransformWrongLength";
    case manifold::Manifold::Error::RunIndexWrongLength: return "RunIndexWrongLength";
    case manifold::Manifold::Error::FaceIDWrongLength: return "FaceIDWrongLength";
    default: return "unknown";
  }
}

const char* opTypeToString(manifold::Manifold::OpType opType) {
  switch (opType) {
    case manifold::Manifold::OpType::Add: return "Add";
    case manifold::Manifold::OpType::Intersect: return "AddIntersect";
    case manifold::Manifold::OpType::Subtract: return "Subtract";
    default: return "unknown";
  }
}

// template <class TriangleMesh>
// std::shared_ptr<manifold::Mesh> meshFromSurfaceMesh(const TriangleMesh& sm)
// {
//   auto mesh = make_shared<manifold::Mesh>();
//   mesh->vertPos.resize(sm.number_of_vertices());
//   mesh->triVerts.resize(sm.number_of_faces());

//   int vidx = -1;
//   for (auto vert : sm.vertices()) {
//     vidx++;
//     auto& v = sm.point(vert);
//     double x = CGAL::to_double(v.x());
//     double y = CGAL::to_double(v.y());
//     double z = CGAL::to_double(v.z());
//     mesh->vertPos[vidx] = glm::vec3((float) x, (float) y, (float) z);
//   }
  
//   int fidx = -1;
//   for (auto& f : sm.faces()) {
//     fidx++;
//     CGAL::Vertex_around_target_circulator<TriangleMesh> vit(sm.halfedge(f), sm), vend(vit);
//     auto i0 = *(vit++);
//     if (vit == vend) continue; // Only 1 vertex
//     auto i1 = *(vit++);
//     if (vit == vend) continue; // Only 2 vertices
//     auto i2 = *(vit++);
//     if (vit != vend) continue; // Mode than 3 vertices
    
//     mesh->triVerts[fidx++] = glm::ivec3(i0, i1, i2);
//   }
//   return mesh;
// }

// template std::shared_ptr<manifold::Mesh> meshFromSurfaceMesh(const CGAL_DoubleMesh& mesh);

std::shared_ptr<manifold::Mesh> meshFromPolySet(const PolySet& ps) {
  IndexedMesh im;
  {
    PolySet triangulated(3);
    PolySetUtils::tessellate_faces(ps, triangulated);
    im.append_geometry(triangulated);
  }

  auto numfaces = im.numfaces;
  const auto &vertices = im.vertices.getArray();
  const auto &indices = im.indices;

  auto mesh = make_shared<manifold::Mesh>();
  mesh->vertPos.resize(vertices.size());
  mesh->triVerts.resize(numfaces);
  for (size_t i = 0, n = vertices.size(); i < n; i++) {
    const auto &v = vertices[i];
    mesh->vertPos[i] = glm::vec3((float) v.x(), (float) v.y(), (float) v.z());
  }
  const auto vertexCount = mesh->vertPos.size();
  assert(indices.size() == numfaces * 4);
  for (size_t i = 0; i < numfaces; i++) {
    auto offset = i * 4; // 3 indices of triangle then -1.
    auto i0 = indices[offset];
    auto i1 = indices[offset + 1];
    auto i2 = indices[offset + 2];
    assert(indices[offset + 3] == -1);
    assert(i0 >= 0 && i0 < vertexCount &&
           i1 >= 0 && i1 < vertexCount &&
           i2 >= 0 && i2 < vertexCount);
    assert(i0 != i1 && i0 != i2 && i1 != i2);
    mesh->triVerts[i] = glm::ivec3(i0, i1, i2);
  }
  return mesh;
}

std::shared_ptr<ManifoldGeometry> createMutableManifoldFromPolySet(const PolySet& ps) {
#if 1
  PolySet psq(ps);
  std::vector<Vector3d> points3d;
  psq.quantizeVertices(&points3d);
  PolySet ps_tri(3, psq.convexValue());
  PolySetUtils::tessellate_faces(psq, ps_tri);
  
  CGAL_DoubleMesh m;

  if (ps_tri.is_convex()) {
    using K = CGAL::Epick;
    // Collect point cloud
    std::vector<K::Point_3> points(points3d.size());
    for (size_t i = 0, n = points3d.size(); i < n; i++) {
      points[i] = vector_convert<K::Point_3>(points3d[i]);
    }
    if (points.size() <= 3) return make_shared<ManifoldGeometry>();

    // Apply hull
    CGAL::Surface_mesh<CGAL::Point_3<K>> r;
    CGAL::convex_hull_3(points.begin(), points.end(), r);
    // auto r_exact = make_shared<CGAL_HybridMesh>();
    // copyMesh(r, *r_exact);
    CGALUtils::copyMesh(r, m);
  } else {
    CGALUtils::createMeshFromPolySet(ps_tri, m);
  }

  if (!ps_tri.is_convex()) {
    if (CGALUtils::isClosed(m)) {
      CGALUtils::orientToBoundAVolume(m);
    } else {
      LOG(message_group::Error, Location::NONE, "", "[manifold] Input mesh is not closed!");
    }
  }

  PolySet pps(3, ps.convexValue());
  // TODO: create method to build a manifold::Mesh from a CGAL::Surface_mesh
  CGALUtils::createPolySetFromMesh(m, pps);
  auto mesh = meshFromPolySet(pps);
#elif 0
  // Here we get orientation and other tweaks for free:
  auto pps = CGALUtils::createHybridPolyhedronFromPolySet(ps)->toPolySet();
  auto mesh = meshFromPolySet(*pps);
#else
  auto mesh = meshFromPolySet(ps);
#endif
  auto mani = std::make_shared<manifold::Manifold>(*mesh);
  auto status = mani->Status();
  if (status != manifold::Manifold::Error::NoError) {
    LOG(message_group::Error, Location::NONE, "",
        "[manifold] PolySet -> Manifold conversion failed: %1$s", 
        ManifoldUtils::statusToString(status));
  }
  return std::make_shared<ManifoldGeometry>(mani);
}

std::shared_ptr<ManifoldGeometry> createMutableManifoldFromGeometry(const std::shared_ptr<const Geometry>& geom) {
  if (auto mani = dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    auto result = std::make_shared<ManifoldGeometry>(*mani);
    // result->transform(transform);
    return result;
  }

  auto ps = CGALUtils::getGeometryAsPolySet(geom);
  if (ps) {
    return createMutableManifoldFromPolySet(*ps);
  }
  
  return nullptr;
}

}; // namespace ManifoldUtils