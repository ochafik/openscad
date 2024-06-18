/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *  Copyright (C) 2021      Konstantin Podsvirov <konstantin@podsvirov.pro>
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
#include "linalg.h"
#include "Feature.h"
#include "ManifoldGeometry.h"
#include "Reindexer.h"
#include "PolySet.h"
#include "PolySetUtils.h"

uint8_t clamp_color_channel(float value)
{
  if (value < 0) return 0;
  if (value > 1) return 255;
  return (uint8_t)(value * 255);
}

void export_off(const std::shared_ptr<const Geometry>& geom, std::ostream& output)
{
  auto output_header = [&](size_t numverts, size_t numfaces) {
    output << "OFF " << numverts << " " << numfaces << " 0\n";
    if (Feature::ExperimentalPredictibleOutput.is_enabled()) {
      output << "# Vertices\n";
    }
  };
  auto output_faces = [&](std::shared_ptr<const PolySet>& ps, const std::function<size_t(size_t)>& get_vert_index) {
    if (Feature::ExperimentalPredictibleOutput.is_enabled()) {
      ps = createSortedPolySet(*ps);
    }
    auto & color = ps->getColor();
    auto has_color = Feature::ExperimentalColors.is_enabled() && color.isValid();
    
    for (size_t i = 0; i < ps->indices.size(); ++i) {
      int nverts = ps->indices[i].size();
      output << nverts;
      for (size_t n = 0; n < nverts; ++n) output << " " << get_vert_index(ps->indices[i][n]);
      if (has_color) {
        auto r = clamp_color_channel(color[0]);
        auto g = clamp_color_channel(color[1]);
        auto b = clamp_color_channel(color[2]);
        auto a = clamp_color_channel(color[3]);
        output << " " << (int)r << " " << (int)g << " " << (int)b;
        // Alpha channel is read by apps like MeshLab.
        if (a != 255) output << " " << (int)a;
      }
      output << "\n";
    }
  };
  
  if (Feature::ExperimentalColors.is_enabled()) {
    Reindexer<Vector3d> reindexer;
    std::vector<std::shared_ptr<const PolySet>> polysets;
    std::vector<std::unordered_map<size_t, size_t>> polyset_reindex_maps;
    size_t face_count = 0;

    std::function<void(const std::shared_ptr<const Geometry>& geom)> visit = [&](const std::shared_ptr<const Geometry>& geom) {
      if (auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
        polysets.push_back(ps);
        polyset_reindex_maps.resize(polyset_reindex_maps.size() + 1);
        auto & map = polyset_reindex_maps.back();
        for (size_t i = 0; i < ps->vertices.size(); ++i) {
          map[i] = reindexer.lookup(ps->vertices[i]);
        }
        face_count += ps->indices.size();
      } else if (auto list = std::dynamic_pointer_cast<const GeometryList>(geom)) {
        for (const auto & item : list->getChildren()) {
          visit(item.second);
        }
      } else if (auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
        for (const auto & ps : mani->toPolySets()) {
          visit(ps);
        }
      } else if (auto ps = PolySetUtils::getGeometryAsPolySet(geom)) {
        visit(ps);
      } else {
        throw std::runtime_error("Unsupported geometry type");
      }
    };
    visit(geom);

    output_header(reindexer.size(), face_count);
    for (const auto & v : reindexer.getArray()) {
      output << v[0] << " " << v[1] << " " << v[2] << " " << "\n";
    }
    for (int i = 0, n = polysets.size(); i < n; ++i) {
      const auto& v = polysets[i]->vertices;
      const auto& map = polyset_reindex_maps[i];
      output_faces(polysets[i], [&](size_t i) { return map.at(i); });
    }
  } else {
    auto ps = PolySetUtils::getGeometryAsPolySet(geom);
    const auto& v = ps->vertices;
    size_t numverts = v.size();
    output_header(numverts, ps->indices.size());
    for (size_t i = 0; i < numverts; ++i) {
      output << v[i][0] << " " << v[i][1] << " " << v[i][2] << " " << "\n";
    }
    output_faces(ps, [](size_t i) { return i; });
  };
}
