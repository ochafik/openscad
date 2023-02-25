// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "ManifoldGeometry.h"
#include "manifold.h"
#include "IndexedMesh.h"
#include "cgalutils.h"
#include "manifoldutils.h"

ManifoldGeometry::ManifoldGeometry() : object(make_shared<manifold::Manifold>()) {}
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

std::shared_ptr<const PolySet> ManifoldGeometry::toPolySet() const {
  auto ps = std::make_shared<PolySet>(3);
  if (this->object) {
    manifold::Mesh mesh = this->object->GetMesh();
    ps->reserve(mesh.triVerts.size());
    Polygon poly(3);
    for (const auto &tv : mesh.triVerts) {
      for (const int j : {0, 1, 2}) {
        poly[j] = vector_convert<Vector3d>(mesh.vertPos[tv[j]]);
      }
      ps->append_poly(poly);
    }
  }
  return ps;
}

std::string describeForDebug(const manifold::Manifold& mani) {
  std::ostringstream stream;
  stream
    << "{"
    << (mani.Status() == manifold::Manifold::Error::NoError ? "OK" : ManifoldUtils::statusToString(mani.Status())) << " "
    << mani.NumTri() << " facets"
    << "}";
  return stream.str();
}

const char* opName(manifold::Manifold::OpType opType) {
  switch (opType) {
    case manifold::Manifold::OpType::Add: return "Add";
    case manifold::Manifold::OpType::Intersect: return "AddIntersect";
    case manifold::Manifold::OpType::Subtract: return "Subtract";
    default: return "unknown";
  }
}

void binOp(ManifoldGeometry& lhs, const ManifoldGeometry& rhs, manifold::Manifold::OpType opType) {
  if (!lhs.object || !rhs.object) {
    assert(false && "empty operands!");
    return;
  }
  
  auto lhsd = describeForDebug(*lhs.object), rhsd = describeForDebug(*rhs.object);
  lhs.object = make_shared<manifold::Manifold>(std::move(lhs.object->Boolean(*rhs.object, opType)));
  auto resd = describeForDebug(*lhs.object);
  LOG(message_group::None, Location::NONE, "",
        "[manifold] %1$s %2$s %3$s -> %4$s",
        lhsd, opName(opType), rhsd, resd);
}


void ManifoldGeometry::operator+=(ManifoldGeometry& other) {
  binOp(*this, other, manifold::Manifold::OpType::Add);
}
void ManifoldGeometry::operator*=(ManifoldGeometry& other) {
  binOp(*this, other, manifold::Manifold::OpType::Intersect);
}
void ManifoldGeometry::operator-=(ManifoldGeometry& other) {
  binOp(*this, other, manifold::Manifold::OpType::Subtract);
}

void ManifoldGeometry::minkowski(ManifoldGeometry& other) {
  assert(false && "not implemented");
}

void ManifoldGeometry::transform(const Transform3d& mat) {
  if (!this->object) {
    assert(false && "empty operands!");
    return;
  }
  glm::mat4x3 glMat;
  // TODO: convert mat to glMat
  assert(false && "not implemented");
                                
  this->object = make_shared<manifold::Manifold>(std::move(this->object->Transform(glMat)));
}

void ManifoldGeometry::resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) {
  assert(false && "not implemented");
}

// /*! Iterate over all vertices' points until the function returns true (for done). */
void ManifoldGeometry::foreachVertexUntilTrue(const std::function<bool(const glm::vec3& pt)>& f) const {
  if (!this->object) {
    assert(false && "empty operands!");
    return;
  }
  // TODO: send PR to std::move the result of Manifold::GetMesh();
  auto mesh = this->object->GetMesh();
  for (const auto &pt : mesh.vertPos) {
    if (f(pt)) {
      return;
    }
  }
}
