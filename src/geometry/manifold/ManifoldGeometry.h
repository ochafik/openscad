// Portions of this file are Copyright 2023 Google LLC, and licensed under GPL2+. See COPYING.
#pragma once

#include "Geometry.h"
#include <glm/glm.hpp>
#include "linalg.h"
#include "manifold.h"
#include <map>

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
  ManifoldGeometry(const std::shared_ptr<const manifold::Manifold>& object, const std::map<uint32_t, Color4f> & originalIDToColor = std::map<uint32_t, Color4f>());
  ManifoldGeometry(const ManifoldGeometry& other) = default;
  ManifoldGeometry& operator=(const ManifoldGeometry& other);

  [[nodiscard]] bool isEmpty() const override;
  [[nodiscard]] size_t numFacets() const override;
  [[nodiscard]] size_t numVertices() const;
  [[nodiscard]] bool isManifold() const;
  [[nodiscard]] bool isValid() const;
  void clear();

  [[nodiscard]] size_t memsize() const override;
  [[nodiscard]] BoundingBox getBoundingBox() const override;

  [[nodiscard]] std::string dump() const override;
  [[nodiscard]] unsigned int getDimension() const override { return 3; }
  [[nodiscard]] std::unique_ptr<Geometry> copy() const override;

  [[nodiscard]] std::shared_ptr<PolySet> toPolySet() const;
  [[nodiscard]] std::vector<std::shared_ptr<PolySet>> toPolySets() const;

  template <class Polyhedron>
  [[nodiscard]] std::shared_ptr<Polyhedron> toPolyhedron() const;

  /*! union. */
  ManifoldGeometry operator+(const ManifoldGeometry& other) const;
  /*! intersection. */
  ManifoldGeometry operator*(const ManifoldGeometry& other) const;
  /*! difference. */
  ManifoldGeometry operator-(const ManifoldGeometry& other) const;
  /*! minkowksi operation. */
  ManifoldGeometry minkowski(const ManifoldGeometry& other) const;

  Polygon2d slice() const;
  Polygon2d project() const;

  void transform(const Transform3d& mat) override;
  void setColor(const Color4f& c) override;
  void resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) override;

  /*! Iterate over all vertices' points until the function returns true (for done). */
  void foreachVertexUntilTrue(const std::function<bool(const glm::vec3& pt)>& f) const;

  const manifold::Manifold& getManifold() const;
  const std::map<uint32_t, Color4f>& getOriginalIDToColor() const { return originalIDToColor_; }

private:
  ManifoldGeometry binOp(const ManifoldGeometry& lhs, const ManifoldGeometry& rhs, manifold::OpType opType) const;

  std::shared_ptr<const manifold::Manifold> manifold_;
  std::map<uint32_t, Color4f> originalIDToColor_;
};
