#pragma once

// #include <memory>
#include "Geometry.h"
#include "enums.h"
#include "ManifoldGeometry.h"
#include "manifold.h"

// class ManifoldGeometry;
class PolySet;
// class Transform3d;

namespace manifold {
  class Manifold;
  class Mesh;
};

namespace ManifoldUtils {

  const char* statusToString(manifold::Manifold::Error status);
  const char* opTypeToString(manifold::Manifold::OpType opType);

  std::shared_ptr<manifold::Mesh> meshFromPolySet(const PolySet& ps);
  // template <class TriangleMesh>
  // std::shared_ptr<manifold::Mesh> meshFromSurfaceMesh(const TriangleMesh& sm);

  std::shared_ptr<ManifoldGeometry> createMutableManifoldFromPolySet(const PolySet& ps);
  std::shared_ptr<ManifoldGeometry> createMutableManifoldFromGeometry(const std::shared_ptr<const Geometry>& geom);

  std::shared_ptr<const Geometry> applyUnion3DManifold(
    const Geometry::Geometries::const_iterator& chbegin,
    const Geometry::Geometries::const_iterator& chend);

  std::shared_ptr<const Geometry> applyOperator3DManifold(const Geometry::Geometries& children, OpenSCADOperator op);

  std::shared_ptr<const Geometry> applyMinkowskiManifold(const Geometry::Geometries& children);
};