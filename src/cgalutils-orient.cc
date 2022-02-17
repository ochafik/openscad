// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"

#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Surface_mesh.h>

namespace CGALUtils {

template <typename Polyhedron>
void orientToBoundAVolume(Polyhedron& polyhedron)
{
  CGAL::Polygon_mesh_processing::orient_to_bound_a_volume(polyhedron);
}

template void orientToBoundAVolume(CGAL_HybridMesh& polyhedron);

} // namespace CGALUtils

