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
bool ManifoldGeometry::isManifold() const {
  if (!this->object) return true;
  return this->object->Status() == manifold::Manifold::Error::NoError;
}
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

template <typename Polyhedron>
class CGALPolyhedronBuilderFromManifold : public CGAL::Modifier_base<typename Polyhedron::HalfedgeDS>
{
  using HDS = typename Polyhedron::HalfedgeDS;
  using CGAL_Polybuilder = CGAL::Polyhedron_incremental_builder_3<typename Polyhedron::HalfedgeDS>;
public:
  using CGALPoint = typename CGAL_Polybuilder::Point_3;

  const manifold::Mesh& mesh;
  CGALPolyhedronBuilderFromManifold(const manifold::Mesh& mesh) : mesh(mesh) { }

  void operator()(HDS& hds) override {
    CGAL_Polybuilder B(hds, true);
  
    B.begin_surface(mesh.vertPos.size(), mesh.triVerts.size());
    for (const auto &v : mesh.vertPos) {
      B.add_vertex(vector_convert<CGALPoint>(v));
    }

    for (const auto &tv : mesh.triVerts) {
      B.begin_facet();
      for (const int j : {0, 1, 2}) {
        B.add_vertex_to_facet(tv[j]);
      }
      B.end_facet();
    }
    B.end_surface();
  }
};

template <class Polyhedron>
shared_ptr<Polyhedron> ManifoldGeometry::toPolyhedron() const
{
  if (!this->object) return nullptr;

  auto p = make_shared<Polyhedron>();
  try {
    manifold::Mesh mesh = this->object->GetMesh();
    CGALPolyhedronBuilderFromManifold<Polyhedron> builder(mesh);
    p->delegate(builder);
  } catch (const CGAL::Assertion_exception& e) {
    LOG(message_group::Error, Location::NONE, "", "CGAL error in CGALUtils::createPolyhedronFromPolySet: %1$s", e.what());
  }
  return p;
}

template shared_ptr<CGAL::Polyhedron_3<CGAL::Epick>> ManifoldGeometry::toPolyhedron() const;
template shared_ptr<CGAL::Polyhedron_3<CGAL::Epeck>> ManifoldGeometry::toPolyhedron() const;

std::string describeForDebug(const manifold::Manifold& mani) {
  std::ostringstream stream;
  auto bbox = mani.BoundingBox();
  stream
    << "{"
    << (mani.Status() == manifold::Manifold::Error::NoError ? "OK" : ManifoldUtils::statusToString(mani.Status())) << " "
    << mani.NumTri() << " facets"
    // << ", bbox {(" << bbox.min.x << ", " << bbox.min.y << ", " << bbox.min.z << ") -> (" << bbox.max.x << ", " << bbox.max.y << ", " << bbox.max.z << ")}"
    << "}";
  return stream.str();
}

void binOp(ManifoldGeometry& lhs, ManifoldGeometry& rhs, manifold::OpType opType) {
  if (!lhs.object || !rhs.object) {
    assert(false && "empty operands!");
    return;
  }

  // if (getenv("MANIFOLD_FORCE_LEAVES")) {
  //   lhs.object->NumTri(); // Force leaf node
  //   rhs.object->NumTri(); // Force leaf node
  // }
  
// #ifdef DEBUG
//   auto lhsd = describeForDebug(*lhs.object), rhsd = describeForDebug(*rhs.object);
// #endif
  lhs.object = make_shared<manifold::Manifold>(std::move(lhs.object->Boolean(*rhs.object, opType)));

  // if (getenv("MANIFOLD_FORCE_LEAVES")) {
  //   lhs.object->NumTri(); // Force leaf node
  // }
  
// #ifdef DEBUG
//   auto resd = describeForDebug(*lhs.object);
//   LOG(message_group::None, Location::NONE, "",
//         "[manifold] %1$s %2$s %3$s -> %4$s",
//         lhsd, ManifoldUtils::opTypeToString(opType), rhsd, resd);
// #endif
}

void ManifoldGeometry::operator+=(ManifoldGeometry& other) {
  binOp(*this, other, manifold::OpType::Add);
}
void ManifoldGeometry::operator*=(ManifoldGeometry& other) {
  binOp(*this, other, manifold::OpType::Intersect);
}
void ManifoldGeometry::operator-=(ManifoldGeometry& other) {
  binOp(*this, other, manifold::OpType::Subtract);
}

void ManifoldGeometry::minkowski(ManifoldGeometry& other) {
  assert(false && "not implemented");
}

void ManifoldGeometry::transform(const Transform3d& mat) {
  if (!this->object) {
    assert(false && "no manifold!");
    return;
  }
  glm::mat4x3 glMat(
    // Column-major ordering
    mat(0, 0), mat(1, 0), mat(2, 0),
    mat(0, 1), mat(1, 1), mat(2, 1),
    mat(0, 2), mat(1, 2), mat(2, 2),
    mat(0, 3), mat(1, 3), mat(2, 3)
  );                            
  this->object = make_shared<manifold::Manifold>(std::move(this->object->Transform(glMat)));
}

BoundingBox ManifoldGeometry::getBoundingBox() const
{
  BoundingBox result;
  if (this->object) {
    manifold::Box bbox = this->object->BoundingBox();
    result.extend(vector_convert<Eigen::Vector3d>(bbox.min));
    result.extend(vector_convert<Eigen::Vector3d>(bbox.max));
  }
  return result;
}

void ManifoldGeometry::resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) {
  this->transform(GeometryUtils::getResizeTransform(this->getBoundingBox(), newsize, autosize));
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
