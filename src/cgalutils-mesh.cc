#include "cgalutils.h"

#include <CGAL/boost/graph/convert_nef_polyhedron_to_polygon_mesh.h>
#include <CGAL/Surface_mesh.h>
#include "CGALHybridPolyhedron.h"
#include "Reindexer.h"
#include "Polygon2d.h"

namespace CGALUtils {

template <typename K>
bool createMeshFromPolySet(const PolySet& ps, CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh)
{
  bool err = false;
  auto num_vertices = ps.numFacets() * 3;
  auto num_facets = ps.numFacets();
  auto num_edges = num_vertices + num_facets + 2; // Euler's formula.
  mesh.reserve(mesh.number_of_vertices() + num_vertices, mesh.number_of_halfedges() + num_edges,
               mesh.number_of_faces() + num_facets);

  typedef typename CGAL::Surface_mesh<CGAL::Point_3<K>>::Vertex_index Vertex_index;
  std::vector<Vertex_index> polygon;

  std::unordered_map<Vector3d, Vertex_index> indices;

  for (const auto& p : ps.polygons) {
    polygon.clear();
    for (auto& v : p) {
      auto size_before = indices.size();
      auto& index = indices[v];
      if (size_before != indices.size()) {
        index = mesh.add_vertex(vector_convert<CGAL::Point_3<K>>(v));
      }
      polygon.push_back(index);
    }
    mesh.add_face(polygon);
  }
  return err;
}

template bool createMeshFromPolySet(const PolySet& ps, CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& mesh);

template <typename K>
bool createPolySetFromMesh(const CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh, PolySet& ps)
{
  bool err = false;
  ps.reserve(ps.numFacets() + mesh.number_of_faces());
  for (auto& f : mesh.faces()) {
    ps.append_poly();

    CGAL::Vertex_around_face_iterator<typename CGAL::Surface_mesh<CGAL::Point_3<K>>> vbegin, vend;
    for (boost::tie(vbegin, vend) = vertices_around_face(mesh.halfedge(f), mesh); vbegin != vend;
         ++vbegin) {
      auto& v = mesh.point(*vbegin);
      // for (auto &v : f) {
      double x = CGAL::to_double(v.x());
      double y = CGAL::to_double(v.y());
      double z = CGAL::to_double(v.z());
      ps.append_vertex(x, y, z);
    }
  }
  return err;
}

template bool createPolySetFromMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& mesh, PolySet& ps);

template <class InputKernel, class OutputKernel>
void copyMesh(
  const CGAL::Surface_mesh<CGAL::Point_3<InputKernel>>& input,
  CGAL::Surface_mesh<CGAL::Point_3<OutputKernel>>& output)
{
  typedef CGAL::Surface_mesh<CGAL::Point_3<InputKernel>> InputMesh;
  typedef CGAL::Surface_mesh<CGAL::Point_3<OutputKernel>> OutputMesh;

  auto converter = getCartesianConverter<InputKernel, OutputKernel>();
  output.reserve(output.number_of_vertices() + input.number_of_vertices(),
                 output.number_of_halfedges() + input.number_of_halfedges(),
                 output.number_of_faces() + input.number_of_faces());

  std::vector<typename CGAL::Surface_mesh<CGAL::Point_3<OutputKernel>>::Vertex_index> polygon;
  std::unordered_map<typename InputMesh::Vertex_index, typename OutputMesh::Vertex_index> reindexer;
  for (auto face : input.faces()) {
    polygon.clear();

    CGAL::Vertex_around_face_iterator<typename CGAL::Surface_mesh<CGAL::Point_3<InputKernel>>>
    vbegin, vend;
    for (boost::tie(vbegin, vend) = vertices_around_face(input.halfedge(face), input);
         vbegin != vend; ++vbegin) {
      auto input_vertex = *vbegin;
      auto size_before = reindexer.size();
      auto& output_vertex = reindexer[input_vertex];
      if (size_before != reindexer.size()) {
        output_vertex = output.add_vertex(converter(input.point(input_vertex)));
      }
      polygon.push_back(output_vertex);
    }
    output.add_face(polygon);
  }
}

template void copyMesh(
  const CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& input,
  CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& output);
template void copyMesh(
  const CGAL::Surface_mesh<CGAL_Point_3>& input,
  CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& output);
template void copyMesh(
  const CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& input,
  CGAL::Surface_mesh<CGAL_Point_3>& output);
template void copyMesh(
  const CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epick>>& input,
  CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epeck>>& output);

template <typename K>
void convertNefPolyhedronToTriangleMesh(const CGAL::Nef_polyhedron_3<K>& nef, CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh)
{
  CGAL::convert_nef_polyhedron_to_polygon_mesh(nef, mesh, /* triangulate_all_faces */ true);
}

template void convertNefPolyhedronToTriangleMesh(const CGAL::Nef_polyhedron_3<CGAL_Kernel3>& nef, CGAL::Surface_mesh<CGAL::Point_3<CGAL_Kernel3>>& mesh);
template void convertNefPolyhedronToTriangleMesh(const CGAL::Nef_polyhedron_3<CGAL_HybridKernel3>& nef, CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& mesh);

/**
 * Will force lazy coordinates to be exact to avoid subsequent performance issues
 * (only if the kernel is lazy), and will also collect the mesh's garbage if applicable.
 */
void cleanupMesh(CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& mesh, bool is_corefinement_result)
{
  mesh.collect_garbage();
#if FAST_CSG_KERNEL_IS_LAZY
  // If exact corefinement callbacks are enabled, no need to make numbers exact here again.
  auto make_exact =
    Feature::ExperimentalFastCsgExactCorefinementCallback.is_enabled()
      ? !is_corefinement_result
      : Feature::ExperimentalFastCsgExact.is_enabled();

  if (make_exact) {
    for (auto v : mesh.vertices()) {
      auto& pt = mesh.point(v);
      CGAL::exact(pt.x());
      CGAL::exact(pt.y());
      CGAL::exact(pt.z());
    }
  }
#endif // FAST_CSG_KERNEL_IS_LAZY
}

std::pair<size_t, std::shared_ptr<const CGAL_HybridMesh>> get2dOr3dMeshFromGeometry(const std::shared_ptr<const Geometry> &geomRef)
{
  auto geom = geomRef;

  size_t dimension = 3;
  if (auto polygon2d = dynamic_pointer_cast<const Polygon2d>(geom)) {
    geom = std::shared_ptr<const PolySet>(polygon2d->tessellate());
    dimension = 2;
  } else if (auto ps = dynamic_pointer_cast<const PolySet>(geom)) {
    dimension = ps->getDimension();
  }
  
  if (auto hybrid = createMutableHybridPolyhedronFromGeometry(geom)) {
    return std::make_pair(dimension, hybrid->convertToMesh());
  }
  return std::make_pair(0, shared_ptr<const CGAL_HybridMesh>());
}

template <typename TriangleMesh, typename OutStream>
void meshToSource(const TriangleMesh &tm, size_t dimension, OutStream &out, const std::string &indent)
{
  if (dimension != 2 && dimension != 3) throw 0;

  out << (dimension == 2 ? "polygon" : "polyhedron") << "([\n";
  for (auto &v : tm.vertices()) {
    auto& p = tm.point(v);
    double x = CGAL::to_double(p.x());
    double y = CGAL::to_double(p.y());
    double z = CGAL::to_double(p.z());
    out << indent << "  [" << x << ", " << y;
    if (dimension == 3) out << ", " << z;
    out << "],\n";
  }
  out << indent << "], [\n";
  for (auto &f : tm.faces()) {
    auto first = true;
    out << indent << "  [";
    CGAL::Vertex_around_face_iterator<TriangleMesh> vit, vend;
    for (boost::tie(vit, vend) = vertices_around_face(tm.halfedge(f), tm); vit != vend; ++vit) {
      auto v = *vit;
      if (first) {
        first = false;
      } else {
        out << ", ";
      }
      out << (size_t) v;
    }
    out << indent << "],\n";
  }
  out << indent << "]);\n";
}

template void meshToSource(const CGAL_HybridMesh &tm, size_t dimension, std::ostringstream &o, const std::string &indent);

} // namespace CGALUtils
