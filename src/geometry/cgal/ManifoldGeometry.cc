// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "ManifoldGeometry.h"
#include "manifold.h"
#include "IndexedMesh.h"

using namespace manifold;

std::shared_ptr<manifold::Mesh> meshFromPolySet(const PolySet& ps, const Transform3d &transform) {
  IndexedMesh im;
  im.append_geometry(ps);

  const auto &vertices = im.vertices.getArray();
  auto mesh = make_shared<manifold::Mesh>();
  mesh->vertPos.resize(vertices.size());
  mesh->triVerts.resize(im.indices.size() / 3);
  for (size_t i = 0, n = vertices.size(); i < n; i++) {
    auto v = transform * vertices[i];
    mesh->vertPos[i] = glm::vec3((float) v.x(), (float) v.y(), (float) v.z());
  }
  for (size_t i = 0; i < im.numfaces; i++) {
    auto offset = i * 3;
    mesh->triVerts[i] = glm::ivec3(im.indices[offset], im.indices[offset + 1], im.indices[offset + 2]);
  }
  return mesh;
}


ManifoldGeometry::ManifoldGeometry() {}
ManifoldGeometry::ManifoldGeometry(const shared_ptr<manifold::Manifold>& object) : object(object) {}
ManifoldGeometry::ManifoldGeometry(const ManifoldGeometry& other) : object(other.object) {}
ManifoldGeometry& ManifoldGeometry::operator=(const ManifoldGeometry& other) {
  this->object = other.object;
  return *this;
}

bool ManifoldGeometry::isEmpty() const {
  return !this->object || this->object->IsEmpty();
}
size_t ManifoldGeometry::numFacets() const {
  if (!this->object) return 0;
  return this->object->NumTri();
}
size_t ManifoldGeometry::numVertices() const {
  if (!this->object) return 0;
  return this->object->NumVert();
}
// bool ManifoldGeometry::isManifold() const {}
// bool ManifoldGeometry::isValid() const {}
// void clear();

size_t ManifoldGeometry::memsize() const {
  // TODO
  return 123;
}

std::string ManifoldGeometry::dump() const {
  assert(false && "not implemented");
  return "MANIFOLD";
}

// [[nodiscard]] std::shared_ptr<const PolySet> toPolySet() const;

/*! In-place union (this may also mutate/corefine the other polyhedron). */
void ManifoldGeometry::operator+=(ManifoldGeometry& other) {
  assert(false && "not implemented");
}
/*! In-place intersection (this may also mutate/corefine the other polyhedron). */
void ManifoldGeometry::operator*=(ManifoldGeometry& other) {
  assert(false && "not implemented");
}
/*! In-place difference (this may also mutate/corefine the other polyhedron). */
void ManifoldGeometry::operator-=(ManifoldGeometry& other) {
  assert(false && "not implemented");
}
/*! In-place minkowksi operation. If the other polyhedron is non-convex,
  * it is also modified during the computation, i.e., it is decomposed into convex pieces.
  */
void ManifoldGeometry::minkowski(ManifoldGeometry& other) {
  assert(false && "not implemented");
}
void ManifoldGeometry::transform(const Transform3d& mat) {
  assert(false && "not implemented");
}
void ManifoldGeometry::resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) {
  assert(false && "not implemented");
}

// /*! Iterate over all vertices' points until the function returns true (for done). */
// void foreachVertexUntilTrue(const std::function<bool(const point_t& pt)>& f) const {
//   assert(false && "not implemented");
// }
