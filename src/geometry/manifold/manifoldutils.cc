// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.

#include "manifoldutils.h"
#include "ManifoldGeometry.h"
#include "manifold.h"
#include "IndexedMesh.h"
#include "printutils.h"
#include "cgalutils.h"
#include "PolySetUtils.h"

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

std::shared_ptr<manifold::Mesh> meshFromPolySet(const PolySet& ps, const Transform3d &transform) {
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
    auto v = transform * vertices[i];
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

std::shared_ptr<ManifoldGeometry> createMutableManifoldFromPolySet(const PolySet& ps, const Transform3d &transform) {
  auto mesh = meshFromPolySet(ps, transform);
  auto mani = std::make_shared<manifold::Manifold>(*mesh);
  auto status = mani->Status();
  if (status != manifold::Manifold::Error::NoError) {
    LOG(message_group::Error, Location::NONE, "",
        "[manifold] PolySet -> Manifold conversion failed: %1$s", 
        ManifoldUtils::statusToString(status));
  }
  return std::make_shared<ManifoldGeometry>(mani);
}

std::shared_ptr<ManifoldGeometry> createMutableManifoldFromGeometry(const std::shared_ptr<const Geometry>& geom, const Transform3d &transform) {
  if (auto mani = dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    auto result = std::make_shared<ManifoldGeometry>(*mani);
    // result->transform(transform);
    return result;
  }

  auto ps = CGALUtils::getGeometryAsPolySet(geom);
  if (ps) {
    return createMutableManifoldFromPolySet(*ps, transform);
  }
  
  return nullptr;
}

}; // namespace ManifoldUtils