// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM

#ifdef ENABLE_CGAL

#include "cgal.h"
#include "cgalutils.h"
#include "PolySet.h"
#include "printutils.h"
#include "CGALHybridPolyhedron.h"
#include "node.h"

#include <CGAL/convex_hull_3.h>

#include "memory.h"

#include <map>
#include <queue>
#include <unordered_set>

namespace ManifoldUtils {

/*!
   children cannot contain nullptr objects
 */
shared_ptr<const Geometry> applyMinkowskiManifold(const Geometry::Geometries& children, const Transform3d& transform)
{
  assert(false && "not implemented");
  return nullptr;
}

}  // namespace ManifoldUtils


#endif // ENABLE_CGAL








