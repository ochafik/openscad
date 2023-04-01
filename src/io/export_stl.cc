/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "export.h"
#include "PolySet.h"
#include "PolySetUtils.h"
#include "parallel.h"
#ifdef ENABLE_MANIFOLD
#include "ManifoldGeometry.h"
#include "manifold.h"
#endif

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "CGALHybridPolyhedron.h"
#include "cgal.h"
#include "cgalutils.h"

namespace {

template <class T>
T normalize(T x) {
  return x == -0 ? 0 : x;
}

template <class T>
std::string toString(const T& v)
{
  return STR(normalize(v[0]), " ", normalize(v[1]), " ", normalize(v[2]));
}

Vector3d toVector(const std::array<double, 3>& pt) {
  return {pt[0], pt[1], pt[2]};
}

#if 0
template <class T>
T fromString(const std::string& vertexString);
{
  T v;
  std::istringstream stream{vertexString};
  stream >> v[0] >> v[1] >> v[2];
  return v;
}
#else
template <class T>
T fromString(const std::string& vertexString);

template <>
Vector3f fromString(const std::string& vertexString)
{
  auto cstr = vertexString.c_str();
  return {
    strtof(cstr, (char**)&cstr), 
    strtof(cstr, (char**)&cstr), 
    strtof(cstr, (char**)&cstr)
  };
}

template <>
Vector3d fromString(const std::string& vertexString)
{
  auto cstr = vertexString.c_str();
  return {
    strtod(cstr, (char**)&cstr), 
    strtod(cstr, (char**)&cstr), 
    strtod(cstr, (char**)&cstr)
  };
}
#endif

void write_vector(std::ostream& output, const Vector3f& v) {
  for (int i = 0; i < 3; ++i) {
    static_assert(sizeof(float) == 4, "Need 32 bit float");
    float f = v[i];
    char *fbeg = reinterpret_cast<char *>(&f);
    char data[4];

    uint16_t test = 0x0001;
    if (*reinterpret_cast<char *>(&test) == 1) {
      std::copy(fbeg, fbeg + 4, data);
    } else {
      std::reverse_copy(fbeg, fbeg + 4, data);
    }
    output.write(data, 4);
  }
}

template <class TriangleMesh>
bool get_triangle(const TriangleMesh &tm, typename TriangleMesh::Face_index f, std::array<typename TriangleMesh::Vertex_index, 3> &out) {
  size_t i = 0;
  CGAL::Vertex_around_face_iterator<TriangleMesh> vit, vend;
  for (boost::tie(vit, vend) = vertices_around_face(tm.halfedge(f), tm); vit != vend; ++vit) {
    if (i >= 3) {
      return false;
    }
    out[i++] = *vit;
  }
  return i == 3;
}

template <class TriangleMesh>
uint64_t append_stl(const TriangleMesh& tm, std::ostream& output, bool binary, bool is_triangle = false, bool was_sorted = false)
{
  if (!is_triangle) {
    TriangleMesh tri(tm);
    CGALUtils::triangulateFaces(tri);
    return append_stl(tri, output, binary, /* is_triangle= */ true, was_sorted);
  }
  if (Feature::ExperimentalSortStl.is_enabled() && !was_sorted) {
    TriangleMesh sorted;
    Export::sortMesh(tm, sorted);
    return append_stl(sorted, output, binary, is_triangle, /* was_sorted= */ true);
  }

  auto reorderVertices = [](auto &v0, auto &v1) {
    if (v0 < v1) return false;

    auto tmp = v0;
    v0 = v1;
    v1 = tmp;
    return true;
  };

  std::vector<Vector3f> points(tm.number_of_vertices());
  std::vector<std::string> pointStrings;

  if (binary) {
    std::transform(tm.vertices().begin(), tm.vertices().end(), points.begin(), [&](auto v) {
      return vector_convert<Vector3f>(tm.point(v));
    });
  } else {
    pointStrings.resize(points.size());
    std::transform(tm.vertices().begin(), tm.vertices().end(), points.begin(), [&](auto v) {
      auto &ps = pointStrings[v] = toString(tm.point(v));
      return fromString<Vector3f>(ps);
    });
  }

  uint64_t triangle_count = 0;

  if (binary) {
    std::array<typename TriangleMesh::Vertex_index, 3> triangle_vertices;
    for (auto& f : tm.faces()) {
      get_triangle(tm, f, triangle_vertices);
      auto v0 = triangle_vertices[0];
      auto v1 = triangle_vertices[1];
      auto v2 = triangle_vertices[2];

      auto &p0 = points[v0];
      auto &p1 = points[v1];
      auto &p2 = points[v2];

      auto normal = (p1 - p0).cross(p2 - p0);
      normal.normalize();

      if (!normal.allFinite()) {
        continue;
      }
      triangle_count++;

      // if (distinctPoints) {
      //   if (collinearVertices) {
          // normal << 0, 0, 0;
        // }
      write_vector(output, normal);
      // }
      write_vector(output, p0);
      write_vector(output, p1);
      write_vector(output, p2);
      char attrib[2] = {0, 0};
      output.write(attrib, 2);
    }
  } else {
    std::array<typename TriangleMesh::Vertex_index, 3> triangle_vertices;
    for (auto& f : tm.faces()) {
      get_triangle(tm, f, triangle_vertices);
      
      auto v0 = triangle_vertices[0];
      auto v1 = triangle_vertices[1];
      auto v2 = triangle_vertices[2];

      auto &p0 = points[v0];
      auto &p1 = points[v1];
      auto &p2 = points[v2];

      auto normal = (p1 - p0).cross(p2 - p0);
      normal.normalize();

      if (!normal.allFinite()) {
        continue;
      }
      triangle_count++;

      output << "  facet normal ";
      output << normalize(normal[0]) << " " << normalize(normal[1]) << " " << normalize(normal[2])
              << "\n";
      output << "    outer loop\n";
      output << "      vertex " << pointStrings[v0] << "\n";
      output << "      vertex " << pointStrings[v1] << "\n";
      output << "      vertex " << pointStrings[v2] << "\n";
      output << "    endloop\n";
      output << "  endfacet\n";
    }
  }

  return triangle_count;
}

#ifdef ENABLE_MANIFOLD
uint64_t append_stl(const manifold::Mesh& mesh, std::ostream& output, bool binary, bool was_sorted = false)
{
  if (Feature::ExperimentalSortStl.is_enabled() && !was_sorted) {
    manifold::Mesh sorted;
    Export::sortMesh(mesh, sorted);
    return append_stl(sorted, output, binary, /* was_sorted= */ true);
  }

  std::vector<Vector3f> points(mesh.vertPos.size());
  std::vector<std::string> pointStrings;

  // auto num_vert = mesh.vertPos.size();
  // auto num_tri = mesh.triVerts.size();
  if (binary) {
    std::transform(mesh.vertPos.begin(), mesh.vertPos.end(), points.begin(), [](const auto &p) {
      return vector_convert<Vector3f>(p);
    });
  } else {
    pointStrings.resize(points.size());
    for (size_t i = 0, n = mesh.vertPos.size(); i < n; i++) {
      const auto &p = mesh.vertPos[i];
      auto &ps = pointStrings[i] = toString(p);
      points[i] = fromString<Vector3f>(ps);
    }
  }

  uint64_t triangle_count = 0;

  if (binary) {
    for (const auto& tri : mesh.triVerts) {
      auto v0 = tri[0];
      auto v1 = tri[1];
      auto v2 = tri[2];

      auto &p0 = points[v0];
      auto &p1 = points[v1];
      auto &p2 = points[v2];

      auto normal = (p1 - p0).cross(p2 - p0);
      normal.normalize();

      if (!normal.allFinite()) {
        continue;
      }
      triangle_count++;

      // if (distinctPoints) {
      //   if (collinearVertices) {
          // normal << 0, 0, 0;
        // }
      write_vector(output, normal);
      // }
      write_vector(output, p0);
      write_vector(output, p1);
      write_vector(output, p2);
      char attrib[2] = {0, 0};
      output.write(attrib, 2);
    }
  } else {
    for (const auto& tri : mesh.triVerts) {
      auto v0 = tri[0];
      auto v1 = tri[1];
      auto v2 = tri[2];

      auto &p0 = points[v0];
      auto &p1 = points[v1];
      auto &p2 = points[v2];

      auto normal = (p1 - p0).cross(p2 - p0);
      normal.normalize();

      if (!normal.allFinite()) {
        continue;
      }
      triangle_count++;
      
      output << "  facet normal ";
      output << normalize(normal[0]) << " " << normalize(normal[1]) << " " << normalize(normal[2])
              << "\n";
      output << "    outer loop\n";
      output << "      vertex " << pointStrings[v0] << "\n";
      output << "      vertex " << pointStrings[v1] << "\n";
      output << "      vertex " << pointStrings[v2] << "\n";
      output << "    endloop\n";
      output << "  endfacet\n";
    }
  }

  return triangle_count;
}
#endif // ENABLE_MANIFOLD

uint64_t append_stl_legacy(const PolySet& ps, std::ostream& output, bool binary)
{
  uint64_t triangle_count = 0;
  PolySet triangulated(3);
  PolySetUtils::tessellate_faces(ps, triangulated);

  auto processTriangle = [&](const std::array<Vector3d, 3>& p, std::ostream& output) {
      triangle_count++;
      if (binary) {
        Vector3f p0 = p[0].cast<float>();
        Vector3f p1 = p[1].cast<float>();
        Vector3f p2 = p[2].cast<float>();

        // Ensure 3 distinct vertices.
        if ((p0 != p1) && (p0 != p2) && (p1 != p2)) {
          Vector3f normal = (p1 - p0).cross(p2 - p0);
          normal.normalize();
          if (!normal.allFinite()) {
          // if (!is_finite(normal) || is_nan(normal)) {
            // Collinear vertices.
            normal << 0, 0, 0;
          }
          write_vector(output, normal);
        }
        write_vector(output, p0);
        write_vector(output, p1);
        write_vector(output, p2);
        char attrib[2] = {0, 0};
        output.write(attrib, 2);
      } else { // ascii
        std::array<std::string, 3> vertexStrings;
        std::transform(p.cbegin(), p.cend(), vertexStrings.begin(),
                       toString<Vector3d>);

        if (vertexStrings[0] != vertexStrings[1] &&
            vertexStrings[0] != vertexStrings[2] &&
            vertexStrings[1] != vertexStrings[2]) {

          // The above condition ensures that there are 3 distinct
          // vertices, but they may be collinear. If they are, the unit
          // normal is meaningless so the default value of "0 0 0" can
          // be used. If the vertices are not collinear then the unit
          // normal must be calculated from the components.
          output << "  facet normal ";

          Vector3d p0 = fromString<Vector3d>(vertexStrings[0]);
          Vector3d p1 = fromString<Vector3d>(vertexStrings[1]);
          Vector3d p2 = fromString<Vector3d>(vertexStrings[2]);

          Vector3d normal = (p1 - p0).cross(p2 - p0);
          normal.normalize();
          if (!normal.allFinite()) {
          // if (is_finite(normal) && !is_nan(normal)) {
            // output << normal[0] << " " << normal[1] << " " << normal[2]
            output << normalize(normal[0]) << " " << normalize(normal[1]) << " " << normalize(normal[2])
                   << "\n";
          } else {
            output << "0 0 0\n";
          }
          output << "    outer loop\n";

          for (const auto& vertexString : vertexStrings) {
            output << "      vertex " << vertexString << "\n";
          }
          output << "    endloop\n";
          output << "  endfacet\n";
        }
      }
    };

  if (Feature::ExperimentalSortStl.is_enabled()) {
    Export::ExportMesh exportMesh { triangulated };
    exportMesh.foreach_triangle([&](const auto& pts) {
        processTriangle({ toVector(pts[0]), toVector(pts[1]), toVector(pts[2]) }, output);
        return true;
      });
  } else {
    for (const auto& p : triangulated.polygons) {
      assert(p.size() == 3); // STL only allows triangles
      processTriangle({ p[0], p[1], p[2] }, output);
    }
  }
  return triangle_count;
}

uint64_t append_stl(const PolySet& ps, std::ostream& output, bool binary)
{
  if (binary || Feature::ExperimentalLegacyStl.is_enabled()) {
    return append_stl_legacy(ps, output, binary);
  }
  // CGAL_DoubleMesh tm;
  CGAL_FloatMesh tm;
  CGALUtils::createMeshFromPolySet(ps, tm);
  return append_stl(tm, output, binary);
}

/*!
    Saves the current 3D CGAL Nef polyhedron as STL to the given file.
    The file must be open.
 */
uint64_t append_stl(const CGAL_Nef_polyhedron& root_N, std::ostream& output,
                    bool binary)
{
  if (!root_N.p3->is_simple()) {
    LOG(message_group::Export_Warning, Location::NONE, "", "Exported object may not be a valid 2-manifold and may need repair");
  }

  if (binary || Feature::ExperimentalLegacyStl.is_enabled()) {
    uint64_t triangle_count = 0;
    PolySet ps(3);
    if (!CGALUtils::createPolySetFromNefPolyhedron3(*(root_N.p3), ps)) {
      triangle_count += append_stl(ps, output, binary);
    } else {
      LOG(message_group::Export_Error, Location::NONE, "", "Nef->PolySet failed");
    }
    return triangle_count;
  }

  CGAL_SurfaceMesh mesh;
  CGALUtils::convertNefPolyhedronToTriangleMesh(*root_N.p3, mesh);
  return append_stl(mesh, output, binary, /* is_triangle= */ true);
}

/*!
   Saves the current 3D CGAL Nef polyhedron as STL to the given file.
   The file must be open.
 */
uint64_t append_stl(const CGALHybridPolyhedron& hybrid, std::ostream& output,
                    bool binary)
{
  uint64_t triangle_count = 0;
  if (!hybrid.isManifold()) {
    LOG(message_group::Export_Warning, Location::NONE, "", "Exported object may not be a valid 2-manifold and may need repair");
  }

  // auto ps = hybrid.toPolySet();
  auto tm = CGALHybridPolyhedron(hybrid).convertToMesh();
  if (tm) {
    triangle_count += append_stl(*tm, output, binary);
  } else {
    LOG(message_group::Export_Error, Location::NONE, "", "Nef->PolySet failed");
  }

  return triangle_count;
}

#ifdef ENABLE_MANIFOLD
/*!
   Saves the current 3D Manifold geometry as STL to the given file.
   The file must be open.
 */
uint64_t append_stl(const ManifoldGeometry& mani, std::ostream& output,
                    bool binary)
{
  uint64_t triangle_count = 0;
  if (!mani.isManifold()) {
    LOG(message_group::Export_Warning, Location::NONE, "", "Exported object may not be a valid 2-manifold and may need repair");
  }

  return append_stl(mani.getManifold().GetMesh(), output, binary);

  // auto tm = mani.template toSurfaceMesh<CGAL_FloatMesh>();
  // // auto tm = mani.template toSurfaceMesh<CGAL_DoubleMesh>();
  // if (tm) {
  //   triangle_count += append_stl(*tm, output, binary, /* is_triangle= */ true);
  // } else {
  //   LOG(message_group::Export_Error, Location::NONE, "", "Manifold->PolySet failed");
  // }

  // return triangle_count;
}
#endif


uint64_t append_stl(const shared_ptr<const Geometry>& geom, std::ostream& output,
                    bool binary)
{
  uint64_t triangle_count = 0;
  if (const auto geomlist = dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const Geometry::GeometryItem& item : geomlist->getChildren()) {
      triangle_count += append_stl(item.second, output, binary);
    }
  } else if (const auto N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    triangle_count += append_stl(*N, output, binary);
  } else if (const auto ps = dynamic_pointer_cast<const PolySet>(geom)) {
    triangle_count += append_stl(*ps, output, binary);
  } else if (const auto hybrid = dynamic_pointer_cast<const CGALHybridPolyhedron>(geom)) {
    triangle_count += append_stl(*hybrid, output, binary);
#ifdef ENABLE_MANIFOLD
  } else if (const auto mani = dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    triangle_count += append_stl(*mani, output, binary);
#endif
  } else if (dynamic_pointer_cast<const Polygon2d>(geom)) { //NOLINT(bugprone-branch-clone)
    assert(false && "Unsupported file format");
  } else { //NOLINT(bugprone-branch-clone)
    assert(false && "Not implemented");
  }

  return triangle_count;
}

} // namespace

void export_stl(const shared_ptr<const Geometry>& geom, std::ostream& output,
                bool binary)
{
  if (binary) {
    char header[80] = "OpenSCAD Model\n";
    output.write(header, sizeof(header));
    char tmp_triangle_count[4] = {0, 0, 0, 0}; // We must fill this in below.
    output.write(tmp_triangle_count, 4);
  } else {
    setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output
    output << "solid OpenSCAD_Model\n";
  }

  uint64_t triangle_count = append_stl(geom, output, binary);

  if (binary) {
    // Fill in triangle count.
    output.seekp(80, std::ios_base::beg);
    output.put(triangle_count & 0xff);
    output.put((triangle_count >> 8) & 0xff);
    output.put((triangle_count >> 16) & 0xff);
    output.put((triangle_count >> 24) & 0xff);
    if (triangle_count > 4294967295) {
      LOG(message_group::Export_Error, Location::NONE, "", "Triangle count exceeded 4294967295, so the stl file is not valid");
    }
  } else {
    output << "endsolid OpenSCAD_Model\n";
    setlocale(LC_NUMERIC, ""); // Set default locale
  }
}

#endif // ENABLE_CGAL
