// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM

#ifdef ENABLE_MANIFOLD

#include "manifoldutils.h"
#include "ManifoldGeometry.h"
#include "node.h"
#include "progress.h"
#include "printutils.h"

#include <queue>

namespace ManifoldUtils {

Location getLocation(const std::shared_ptr<const AbstractNode>& node)
{
  return node && node->modinst ? node->modinst->location() : Location::NONE;
}

/*!
   Applies op to all children and returns the result.
   The child list should be guaranteed to contain non-NULL 3D or empty Geometry objects
 */
shared_ptr<const Geometry> applyOperator3DManifold(const Geometry::Geometries& children, OpenSCADOperator op)
{
  shared_ptr<ManifoldGeometry> N;

  // assert(op != OpenSCADOperator::UNION && "use applyUnion3D() instead of applyOperator3D()");
  bool foundFirst = false;

  // try {
    for (const auto& item : children) {
      auto chN = item.second ? createMutableManifoldFromGeometry(item.second) : nullptr;

      // Intersecting something with nothing results in nothing
      if (!chN || chN->isEmpty()) {
        if (op == OpenSCADOperator::INTERSECTION) {
          N = nullptr;
          break;
        }
        if (op == OpenSCADOperator::DIFFERENCE && !foundFirst) {
          N = nullptr;
          break;
        }
        continue;
      }

      // Initialize N with first expected geometric object
      if (!foundFirst) {
        N = chN;
        foundFirst = true;
        continue;
      }

      // empty op <something> => empty
      // if (!N || N->isEmpty()) continue;

      switch (op) {
      case OpenSCADOperator::UNION:
        *N += *chN;
        break;
      case OpenSCADOperator::INTERSECTION:
        *N *= *chN;
        break;
      case OpenSCADOperator::DIFFERENCE:
        *N -= *chN;
        break;
      case OpenSCADOperator::MINKOWSKI:
        N->minkowski(*chN);
        break;
      default:
        LOG(message_group::Error, Location::NONE, "", "Unsupported CGAL operator: %1$d", static_cast<int>(op));
      }
      if (item.first) item.first->progress_report();
    }
  // }
  // // union && difference assert triggered by testdata/scad/bugs/rotate-diff-nonmanifold-crash.scad and testdata/scad/bugs/issue204.scad
  // catch (const CGAL::Failure_exception& e) {
  //   std::string opstr = op == OpenSCADOperator::INTERSECTION ? "intersection" : op == OpenSCADOperator::DIFFERENCE ? "difference" : op == OpenSCADOperator::UNION ? "union" : "UNKNOWN";
  //   LOG(message_group::Error, Location::NONE, "", "CGAL error in CGALUtils::applyOperator3DHybrid %1$s: %2$s", opstr, e.what());

  // }
  return N;
}

};  // namespace ManifoldUtils

#endif // ENABLE_MANIFOLD
