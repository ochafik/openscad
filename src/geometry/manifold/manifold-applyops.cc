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

shared_ptr<const Geometry> applyUnion3DManifold(
  const Geometry::Geometries::const_iterator& chbegin,
  const Geometry::Geometries::const_iterator& chend,
  const Transform3d& transform)
{
  using QueueItem = std::pair<shared_ptr<ManifoldGeometry>, int>;
  struct QueueItemGreater {
    // stable sort for priority_queue by facets, then progress mark
    bool operator()(const QueueItem& lhs, const QueueItem& rhs) const
    {
      size_t l = lhs.first->numFacets();
      size_t r = rhs.first->numFacets();
      return (l > r) || (l == r && lhs.second > rhs.second);
    }
  };

  // try {
    Geometry::Geometries children;
    children.insert(children.end(), chbegin, chend);

    // We'll fill the queue in one go to get linear time construction.
    std::vector<QueueItem> queueItems;
    queueItems.reserve(children.size());

    for (auto& item : children) {
      auto chgeom = item.second;
      if (!chgeom || chgeom->isEmpty()) {
        continue;
      }
      auto poly = createMutableManifoldFromGeometry(chgeom, transform);
      if (!poly) {
        continue;
      }

      auto node_mark = item.first ? item.first->progress_mark : -1;
      queueItems.emplace_back(poly, node_mark);
    }
    // Build the queue in linear time (don't add items one by one!).
    std::priority_queue<QueueItem, std::vector<QueueItem>, QueueItemGreater>
    q(queueItems.begin(), queueItems.end());

    progress_tick();
    while (q.size() > 1) {
      auto p1 = q.top();
      q.pop();
      auto p2 = q.top();
      q.pop();
      assert(p1.first->numFacets() <= p2.first->numFacets());
      // Modify in-place the biggest polyhedron.
      *p2.first += *p1.first;
      q.emplace(p2.first, -1);
      progress_tick();
    }

    if (q.size() == 1) {
      return q.top().first;
    } else {
      return nullptr;
    }
  // } catch (const CGAL::Failure_exception& e) {
  //   LOG(message_group::Error, Location::NONE, "", "CGAL error in CGALUtils::applyUnion3DHybrid: %1$s", e.what());
  // }
  return nullptr;
}

/*!
   Applies op to all children and returns the result.
   The child list should be guaranteed to contain non-NULL 3D or empty Geometry objects
 */
shared_ptr<const Geometry> applyOperator3DManifold(const Geometry::Geometries& children, OpenSCADOperator op, const Transform3d& transform)
{
  shared_ptr<ManifoldGeometry> N;

  assert(op != OpenSCADOperator::UNION && "use applyUnion3D() instead of applyOperator3D()");
  bool foundFirst = false;

  // try {
    for (const auto& item : children) {
      auto chN = item.second ? createMutableManifoldFromGeometry(item.second, transform) : nullptr;
      // Initialize N with first expected geometric object
      if (!foundFirst) {
        if (chN) {
          N = chN;
        } else { // first child geometry might be empty/null
          N = nullptr;
        }
        foundFirst = true;
        continue;
      }

      // Intersecting something with nothing results in nothing
      if (!chN || chN->isEmpty()) {
        if (op == OpenSCADOperator::INTERSECTION) N = nullptr;
        continue;
      }

      // empty op <something> => empty
      if (!N || N->isEmpty()) continue;

      switch (op) {
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