// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#pragma once

#include "Geometry.h"
#include <glm/glm.hpp>

namespace manifold {
  class Manifold;
};

/*! A mutable polyhedron backed by a manifold::Manifold
 */
class ManifoldGeometry : public Geometry
{
public:
  VISITABLE_GEOMETRY();

  ManifoldGeometry();
  ManifoldGeometry(const shared_ptr<manifold::Manifold>& object);
  ManifoldGeometry(const ManifoldGeometry& other);
  ManifoldGeometry& operator=(const ManifoldGeometry& other);

  [[nodiscard]] bool isEmpty() const override;
  [[nodiscard]] size_t numFacets() const override;
  [[nodiscard]] size_t numVertices() const;
  // [[nodiscard]] bool isManifold() const;
  // [[nodiscard]] bool isValid() const;
  // void clear();

  [[nodiscard]] size_t memsize() const override;
  [[nodiscard]] BoundingBox getBoundingBox() const override
  {
    assert(false && "not implemented");
    return {};
  }

  [[nodiscard]] std::string dump() const override;
  [[nodiscard]] unsigned int getDimension() const override { return 3; }
  [[nodiscard]] Geometry *copy() const override { return new ManifoldGeometry(*this); }

  [[nodiscard]] std::shared_ptr<const PolySet> toPolySet() const;

  template <class Polyhedron>
  std::shared_ptr<Polyhedron> toPolyhedron() const;

  /*! In-place union. */
  void operator+=(ManifoldGeometry& other);
  /*! In-place intersection. */
  void operator*=(ManifoldGeometry& other);
  /*! In-place difference. */
  void operator-=(ManifoldGeometry& other);
  /*! In-place minkowksi operation. If the other polyhedron is non-convex,
   * it is also modified during the computation, i.e., it is decomposed into convex pieces.
   */
  void minkowski(ManifoldGeometry& other);
  void transform(const Transform3d& mat) override;
  void resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) override;

  /*! Iterate over all vertices' points until the function returns true (for done). */
  void foreachVertexUntilTrue(const std::function<bool(const glm::vec3& pt)>& f) const;

  shared_ptr<manifold::Manifold> object;
  
private:
};
