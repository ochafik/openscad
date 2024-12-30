// Portions of this file are Copyright 2023 Google LLC, and licensed under GPL2+. See COPYING.
#ifdef ENABLE_MANIFOLD

#include <iterator>
#include <cassert>
#include <list>
#include <exception>
#include <memory>
#include <utility>
#include <vector>
#include <CGAL/convex_hull_3.h>

#include <manifold/manifold.h>
#include "geometry/cgal/cgal.h"
#include "geometry/cgal/cgalutils.h"
#include "geometry/PolySet.h"
#include "utils/printutils.h"
#include "geometry/manifold/manifoldutils.h"
#include "geometry/manifold/ManifoldGeometry.h"
#include "utils/parallel.h"

namespace ManifoldUtils {

/*!
   children cannot contain nullptr objects
 */
std::shared_ptr<const Geometry> applyMinkowskiManifold(const Geometry::Geometries& geomChildren)
{
  using Hull_Point = linalg::vec<double, 3>;
  using Hull_Points = std::vector<Hull_Point>;
  using Polyhedron = CGAL_Polyhedron;

  // auto polyhedronFromGeometry = [](const manifold::Manifold, bool *pIsConvexOut) -> std::shared_ptr<Polyhedron> 
  // {
  //   if (auto ps = dynamic_cast<const PolySet *>(geom.get())) {
  //     auto poly = std::make_shared<Polyhedron>();
  //     CGALUtils::createPolyhedronFromPolySet(*ps, *poly);
  //     if (pIsConvexOut) *pIsConvexOut = ps->isConvex();
  //     return poly;
  //   } else if (auto mani = dynamic_cast<const ManifoldGeometry *>(geom.get())) {
  //     auto poly = mani->toPolyhedron<Polyhedron>();
  //     if (pIsConvexOut) *pIsConvexOut = CGALUtils::is_weakly_convex(*poly);
  //     return poly;
  //   } else {
  //     throw 0;
  //   }
  // };

  std::vector<std::shared_ptr<const ManifoldGeometry>> children;
  for (const auto& item : geomChildren) {
    auto chN = item.second ? createManifoldFromGeometry(item.second) : nullptr;
    if (chN) children.push_back(chN);
  }

  assert(children.size() >= 2);
  auto it = children.begin();
  CGAL::Timer t_tot;
  t_tot.start();
  std::vector<std::shared_ptr<const ManifoldGeometry>> operands { *it, nullptr };
  // operands.emplace_back(*it);
  // operands.emplace_back(nullptr);

  auto getHullPoints = [&](const Polyhedron &poly) {
    std::vector<Hull_Point> out;
    out.reserve(poly.size_of_vertices());
    for (auto pi = poly.vertices_begin(); pi != poly.vertices_end(); ++pi) {
      out.emplace_back(CGALUtils::vector_convert<Hull_Point>(pi->point()));
    }
    return out;
  };
  
  try {
    // Note: we could parallelize more, e.g. compute all decompositions ahead of time instead of doing them 2 by 2,
    // but this could use substantially more memory.

    std::vector<std::vector<Hull_Points>> part_points(2);

    while (++it != children.end()) {
      operands[1] = *it;

      part_points[0].clear();
      part_points[1].clear();

      parallelizable_transform(operands.begin(), operands.begin() + 2, part_points.begin(), [&](const auto &operand) {
        std::vector<Hull_Points> part_points;

        // if (boost::logic::true_value(operand->convexValue())) {
        //   const auto mesh = mani->getManifold().GetMeshGL64();
        //   const auto numVert = mesh.NumVert();

        //   Hull_Points points;
        //   points.reserve(numVert);
        //   for (size_t v = 0; v < numVert; ++v) {
        //     points.emplace_back(mesh.GetVertPos(v));
        //   }
        //   part_points.emplace_back(std::move(points));
        // }
        auto poly = operand->template toPolyhedron<Polyhedron>();
        bool is_convex = boost::logic::indeterminate(operand->convexValue()) ? CGALUtils::is_weakly_convex(*poly) : (bool) operand->convexValue();
        
        if (is_convex) {
          part_points.emplace_back(std::move(getHullPoints(*poly)));
        } else {
          // The CGAL_Nef_polyhedron3 constructor can crash on bad polyhedron, so don't try
          if (!poly->is_valid()) throw 0;
          CGAL_Nef_polyhedron3 decomposed_nef(*poly);
          CGAL::Timer t;
          t.start();
          CGAL::convex_decomposition_3(decomposed_nef);

          // the first volume is the outer volume, which ignored in the decomposition
          CGAL_Nef_polyhedron3::Volume_const_iterator ci = ++decomposed_nef.volumes_begin();
          for (; ci != decomposed_nef.volumes_end(); ++ci) {
            if (ci->mark()) {
              Polyhedron poly;
              decomposed_nef.convert_inner_shell_to_polyhedron(ci->shells_begin(), poly);

              part_points.emplace_back(std::move(getHullPoints(poly)));
            }
          }

          PRINTDB("Minkowski: decomposed into %d convex parts", part_points.size());
          t.stop();
          PRINTDB("Minkowski: decomposition took %f s", t.time());
        }
        return std::move(part_points);
      });
      
      auto combineParts = [&](const Hull_Points &points0, const Hull_Points &points1) -> std::shared_ptr<const ManifoldGeometry> {
        CGAL::Timer t;

        t.start();
        // thread_local
        std::vector<Hull_Point> minkowski_points_vec;

        auto np0 = points0.size();
        auto np1 = points1.size();
        minkowski_points_vec.resize(np0 * np1);
        auto * minkowski_points = minkowski_points_vec.data();
        for (size_t i = 0; i < points0.size(); ++i) {
          auto &p0i = points0[i];
          auto offset = np1 * i;
          for (size_t j = 0; j < points1.size(); ++j) {
            minkowski_points[offset + j] = p0i + points1[j];
          }
        }

        if (minkowski_points_vec.size() <= 3) {
          t.stop();
          return std::make_shared<const ManifoldGeometry>();
        }

        t.stop();
        PRINTDB("Minkowski: Point cloud creation (%d ⨉ %d -> %d) took %f ms", points0.size() % points1.size() % minkowski_points_vec.size() % (t.time() * 1000));
        t.reset();

        t.start();

        auto hull = manifold::Manifold::Hull(minkowski_points_vec);
        t.stop();
        PRINTDB("Minkowski: Computing convex hull took %f s", t.time());
        t.reset();

        return std::make_shared<ManifoldGeometry>(hull, /* convex= */ true);
      };

      std::vector<std::shared_ptr<const ManifoldGeometry>> result_parts(part_points[0].size() * part_points[1].size());
      parallelizable_cross_product_transform(
          part_points[0], part_points[1],
          result_parts.begin(),
          combineParts);

      if (it != std::next(children.begin())) operands[0].reset();

      CGAL::Timer t;
      t.start();
      PRINTDB("Minkowski: Computing union of %d parts", result_parts.size());
      std::vector<manifold::Manifold> result_parts_manifolds;
      for (const auto& part : result_parts) {
        result_parts_manifolds.push_back(part->getManifold());
      }
      auto N = manifold::Manifold::BatchBoolean(result_parts_manifolds, manifold::OpType::Add);

      t.stop();
      PRINTDB("Minkowski: Union done: %f s", t.time());
      t.reset();

      operands[0] = std::make_shared<ManifoldGeometry>(N);
    }

    t_tot.stop();
    PRINTDB("Minkowski: Total execution time %f s", t_tot.time());
    t_tot.reset();
    return operands[0];
  } catch (const std::exception& e) {
    LOG(message_group::Warning,
        "[manifold] Minkowski failed with error, falling back to Nef operation: %1$s\n", e.what());
  } catch (...) {
    LOG(message_group::Warning,
        "[manifold] Minkowski hard-crashed, falling back to Nef operation.");
  }
  return ManifoldUtils::applyOperator3DManifold(geomChildren, OpenSCADOperator::MINKOWSKI);
}

}  // namespace ManifoldUtils

#endif // ENABLE_MANIFOLD
