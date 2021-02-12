// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include <CGAL/Polygon_mesh_processing/orientation.h>

namespace CGALUtils {

void orientToBoundAVolume(CGAL::Polyhedron_3<CGAL::Epeck> &polyhedron)
{
	CGAL::Polygon_mesh_processing::orient_to_bound_a_volume(polyhedron);
}

} // namespace CGALUtils

#endif // FAST_CSG_AVAILABLE