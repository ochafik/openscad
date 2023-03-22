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
#include "printutils.h"
#include "Geometry.h"
#include "cgalutils.h"

#include <fstream>

#ifdef ENABLE_MANIFOLD
#include "manifold.h"
#endif

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

bool canPreview(const FileFormat format) {
  return (format == FileFormat::AST ||
          format == FileFormat::CSG ||
          format == FileFormat::PARAM ||
          format == FileFormat::ECHO ||
          format == FileFormat::TERM ||
          format == FileFormat::PNG);
}

void exportFile(const shared_ptr<const Geometry>& root_geom, std::ostream& output, const ExportInfo& exportInfo)
{
  switch (exportInfo.format) {
  case FileFormat::ASCIISTL:
    export_stl(root_geom, output, false);
    break;
  case FileFormat::STL:
    export_stl(root_geom, output, true);
    break;
  case FileFormat::OBJ:
    export_obj(root_geom, output);
    break;
  case FileFormat::OFF:
    export_off(root_geom, output);
    break;
  case FileFormat::WRL:
    export_wrl(root_geom, output);
    break;
  case FileFormat::AMF:
    export_amf(root_geom, output);
    break;
  case FileFormat::_3MF:
    export_3mf(root_geom, output);
    break;
  case FileFormat::DXF:
    export_dxf(root_geom, output);
    break;
  case FileFormat::SVG:
    export_svg(root_geom, output);
    break;
  case FileFormat::PDF:
    export_pdf(root_geom, output, exportInfo);
    break;
  case FileFormat::NEFDBG:
    export_nefdbg(root_geom, output);
    break;
  case FileFormat::NEF3:
    export_nef3(root_geom, output);
    break;
  default:
    assert(false && "Unknown file format");
  }
}

bool exportFileByNameStdout(const shared_ptr<const Geometry>& root_geom, const ExportInfo& exportInfo)
{
#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  exportFile(root_geom, std::cout, exportInfo);
  return true;
}

bool exportFileByNameStream(const shared_ptr<const Geometry>& root_geom, const ExportInfo& exportInfo)
{
  std::ios::openmode mode = std::ios::out | std::ios::trunc;
  if (exportInfo.format == FileFormat::_3MF || exportInfo.format == FileFormat::STL || exportInfo.format == FileFormat::PDF) {
    mode |= std::ios::binary;
  }
  std::ofstream fstream(exportInfo.name2open, mode);
  if (!fstream.is_open()) {
    LOG(message_group::None, Location::NONE, "", _("Can't open file \"%1$s\" for export"), exportInfo.name2display);
    return false;
  } else {
    bool onerror = false;
    fstream.exceptions(std::ios::badbit | std::ios::failbit);
    try {
      exportFile(root_geom, fstream, exportInfo);
    } catch (std::ios::failure&) {
      onerror = true;
    }
    try { // make sure file closed - resources released
      fstream.close();
    } catch (std::ios::failure&) {
      onerror = true;
    }
    if (onerror) {
      LOG(message_group::Error, Location::NONE, "", _("\"%1$s\" write error. (Disk full?)"), exportInfo.name2display);
    }
    return !onerror;
  }
}

bool exportFileByName(const shared_ptr<const Geometry>& root_geom, const ExportInfo& exportInfo)
{
  bool exportResult = false;
  if (exportInfo.useStdOut) {
    exportResult = exportFileByNameStdout(root_geom, exportInfo);
  } else {
    exportResult = exportFileByNameStream(root_geom, exportInfo);
  }
  return exportResult;
}

namespace Export {

double normalize(double x) {
  return x == -0 ? 0 : x;
}

template <class V>
ExportMesh::Vertex vectorToVertex(const V& pt) {
  return {normalize(pt.x()), normalize(pt.y()), normalize(pt.z())};
}

ExportMesh::ExportMesh(const PolySet& ps)
{
  std::map<Vertex, int> vertexMap;
  std::vector<std::array<int, 3>> triangleIndices;

  for (const auto& pts : ps.polygons) {
    auto pos1 = vertexMap.emplace(std::make_pair(vectorToVertex(pts[0]), vertexMap.size()));
    auto pos2 = vertexMap.emplace(std::make_pair(vectorToVertex(pts[1]), vertexMap.size()));
    auto pos3 = vertexMap.emplace(std::make_pair(vectorToVertex(pts[2]), vertexMap.size()));
    triangleIndices.push_back({pos1.first->second, pos2.first->second, pos3.first->second});
  }

  std::vector<size_t> indexTranslationMap(vertexMap.size());
  vertices.reserve(vertexMap.size());

  size_t index = 0;
  for (const auto& e : vertexMap) {
    vertices.push_back(e.first);
    indexTranslationMap[e.second] = index++;
  }

  triangles.reserve(triangleIndices.size());
  for (const auto& i : triangleIndices) {
    triangles.emplace_back(indexTranslationMap[i[0]], indexTranslationMap[i[1]], indexTranslationMap[i[2]]);
  }
  std::sort(triangles.begin(), triangles.end(), [](const auto& t1, const auto& t2) -> bool {
      return t1.key < t2.key;
    });
}

template <class TriangleMesh>
void sortMesh(const TriangleMesh& tm, TriangleMesh& out)
{
  using Vertex_index = typename TriangleMesh::Vertex_index;
  using Point = typename TriangleMesh::Point;

  auto comparePoints = [](const auto &a, const auto &b) {
    auto d = a.x() - b.x();
    if (d != 0) return d < 0;

    d = a.y() - b.y();
    if (d != 0) return d < 0;

    return a.z() < b.z();
  };

  assert(out.is_empty());
  out.reserve(tm.number_of_vertices(), tm.number_of_edges(), tm.number_of_faces());

  std::vector<std::pair<Vertex_index, size_t>> vertex_indices(tm.number_of_vertices());
  for (auto v : tm.vertices()) {
    vertex_indices[v] = std::make_pair(v, v);
  }
  std::sort(vertex_indices.begin(), vertex_indices.end(), [&](const auto &p1, const auto &p2) {
    return comparePoints(tm.point(p1.first), tm.point(p2.first));
  });

  std::vector<Vertex_index> vertex_map(vertex_indices.size());
  for (size_t i = 0, n = vertex_indices.size(); i < n; i++) {
    const auto &pair = vertex_indices[i];
    const auto &p = tm.point(pair.first);
    vertex_map[pair.second] = out.add_vertex(vector_convert<Point>(p));
  }

  std::vector<Triangle<Vertex_index>> triangles;
  triangles.reserve(tm.number_of_faces());
  
  std::array<Vertex_index, 3> triangle;
  for (auto f : tm.faces()) {
    size_t i = 0;
    CGAL::Vertex_around_face_iterator<TriangleMesh> it, end;
    for (boost::tie(it, end) = vertices_around_face(tm.halfedge(f), tm); it != end;
          ++it) {
      if (i >= 3) {
        assert(false && !"Mesh is not triangular!");
        break;
      }
      triangle[i++] = vertex_map[*it];
    }
    triangles.emplace_back(triangle[0], triangle[1], triangle[2]);
  }

  std::sort(triangles.begin(), triangles.end(), [](const auto& t1, const auto& t2) -> bool {
      return t1.key < t2.key;
    });
  for (auto &t : triangles) {
    out.add_face(t.key[0], t.key[1], t.key[2]);
  }
}

template void sortMesh(const CGAL_DoubleMesh& tm, CGAL_DoubleMesh &out);
template void sortMesh(const CGAL_FloatMesh& tm, CGAL_FloatMesh &out);
template void sortMesh(const CGAL_HybridMesh& tm, CGAL_HybridMesh &out);
template void sortMesh(const CGAL_SurfaceMesh& tm, CGAL_SurfaceMesh &out);

#ifdef ENABLE_MANIFOLD
template <>
void sortMesh(const manifold::Mesh& mesh, manifold::Mesh& out)
{
  auto comparePoints = [](const auto &a, const auto &b) {
    auto d = a.x - b.x;
    if (d) return d < 0;

    d = a.y - b.y;
    if (d) return d < 0;

    return a.z < b.z;
  };

  assert(out.vertPos.empty() && out.triVerts.empty());
  auto num_vert = mesh.vertPos.size();
  auto num_tri = mesh.triVerts.size();
  out.vertPos.reserve(num_vert);
  out.triVerts.reserve(num_tri);

  std::vector<std::pair<size_t, size_t>> vertex_indices(num_vert);
  for (size_t i = 0; i < num_vert; i++) {
    vertex_indices[i] = std::make_pair(i, i);
  }
  std::sort(vertex_indices.begin(), vertex_indices.end(), [&](const auto &p1, const auto &p2) {
    return comparePoints(mesh.vertPos[p1.first], mesh.vertPos[p2.first]);
  });

  std::vector<size_t> vertex_map(num_vert);
  for (size_t i = 0; i < num_vert; i++) {
    const auto &pair = vertex_indices[i];
    const auto &p = mesh.vertPos[pair.first];
    vertex_map[pair.second] = out.vertPos.size();
    out.vertPos.emplace_back(normalize(p[0]), normalize(p[1]), normalize(p[2]));
  }

  std::vector<Triangle<int>> triangles;
  triangles.reserve(num_tri);
  for (auto &tri : mesh.triVerts) {
    triangles.emplace_back(tri[0], tri[1], tri[2]);
  }

  std::sort(triangles.begin(), triangles.end(), [](const auto& t1, const auto& t2) -> bool {
      return t1.key < t2.key;
    });
  for (auto &t : triangles) {
    out.triVerts.emplace_back(t.key[0], t.key[1], t.key[2]);
  }
}
#endif

bool ExportMesh::foreach_vertex(const std::function<bool(const Vertex&)>& callback) const
{
  for (const auto& v : vertices) {
    if (!callback(v)) {
      return false;
    }
  }
  return true;
}

bool ExportMesh::foreach_indexed_triangle(const std::function<bool(const std::array<size_t, 3>&)>& callback) const
{
  for (const auto& t : triangles) {
    if (!callback(t.key)) {
      return false;
    }
  }
  return true;
}

bool ExportMesh::foreach_triangle(const std::function<bool(const std::array<std::array<double, 3>, 3>&)>& callback) const
{
  for (const auto& t : triangles) {
    auto& v0 = vertices[t.key[0]];
    auto& v1 = vertices[t.key[1]];
    auto& v2 = vertices[t.key[2]];
    if (!callback({ v0, v1, v2 })) {
      return false;
    }
  }
  return true;
}

} // namespace Export
