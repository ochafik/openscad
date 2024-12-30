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
std::shared_ptr<const Geometry> applyMinkowskiManifold(const Geometry::Geometries& children)
{
  using Hull_Point = linalg::vec<double, 3>;
  using Hull_Points = std::vector<Hull_Point>;
  using Polyhedron = CGAL_Polyhedron;
  using Nef = CGAL_Nef_polyhedron3;

  assert(children.size() >= 2);
  auto it = children.begin();
  CGAL::Timer t_tot;
  t_tot.start();
  std::vector<std::shared_ptr<const Geometry>> operands = {it->second, std::shared_ptr<const Geometry>()};

  try {
    // Note: we could parallelize more, e.g. compute all decompositions ahead of time instead of doing them 2 by 2,
    // but this could use substantially more memory.
    while (++it != children.end()) {
      operands[1] = it->second;

      std::vector<std::list<Hull_Points>> part_points(2);

      parallelizable_transform(operands.begin(), operands.begin() + 2, part_points.begin(), [&](const auto &operand) {
        // List of points of each convex part of the operand.
        std::list<Hull_Points> part_points;

        // bool is_convex = false;
        std::shared_ptr<Polyhedron> poly;
        boost::tribool convex = boost::indeterminate;
        
        if (auto ps = dynamic_cast<const PolySet *>(operand.get())) {
          // poly = std::make_shared<Polyhedron>();
          // CGALUtils::createPolyhedronFromPolySet(*ps, *poly);
          // is_convex = ps->isConvex();
          if (!ps->isEmpty()) {
            if (ps->convexValue()) {
              Hull_Points points;
              points.reserve(ps->vertices.size());
              for (const auto &p : ps->vertices) {
                points.emplace_back(CGALUtils::vector_convert<Hull_Point>(p));
              }
              part_points.emplace_back(std::move(points));
            } else {
              poly = std::make_shared<Polyhedron>();
              CGALUtils::createPolyhedronFromPolySet(*ps, *poly);
            }
          }
        } else if (auto mani = dynamic_cast<const ManifoldGeometry *>(operand.get())) {
          // poly = mani->toPolyhedron<Polyhedron>();
          // if (mani->convexValue() == boost::indeterminate) {
          //   is_convex = CGALUtils::is_weakly_convex(*poly);
          // } else {
          //   is_convex = (bool) mani->convexValue();
          // }
          // is_convex = (mani->convexValue() == boost::tribool::true_value) || CGALUtils::is_weakly_convex(*poly);
          auto convex = mani->convexValue();
          // if (convex == boost::indeterminate) {
          //   poly = mani->toPolyhedron<Polyhedron>();
          //   convex = CGALUtils::is_weakly_convex(*poly);
          // }
          
          if (convex == boost::tribool::true_value) {
            poly.reset();
            const auto mesh = mani->getManifold().GetMeshGL64();
            const auto numVert = mesh.NumVert();

            Hull_Points points;
            points.reserve(numVert);
            for (size_t v = 0; v < numVert; ++v) {
              points.emplace_back(mesh.GetVertPos(v));
            }
            part_points.emplace_back(std::move(points));
          } else {
            poly = mani->toPolyhedron<Polyhedron>();
          }
        } else {
          throw 0;
        }

        if (poly) {
          if (!poly->is_valid()) throw 0;
          if (CGALUtils::is_weakly_convex(*poly)) {
          // if (is_convex) {
            Hull_Points points;
            points.reserve(poly->size_of_vertices());
            for (auto pi = poly->vertices_begin(); pi != poly->vertices_end(); ++pi) {
              points.emplace_back(CGALUtils::vector_convert<Hull_Point>(pi->point()));
            }
            part_points.emplace_back(std::move(points));
          } else {

            CGAL::Timer t;
            t.start();
            Nef decomposed_nef(*poly);
            CGAL::convex_decomposition_3(decomposed_nef);

            struct VertexCollector {
              Hull_Points points;
              std::unordered_set<Nef::Vertex_const_handle> vertices_seen;

              void clear() {
                points.clear();
                vertices_seen.clear();
              }
              
              void visit(Nef::Vertex_const_handle v) {
                if (vertices_seen.insert(v).second) {
                  points.push_back(CGALUtils::vector_convert<Hull_Point>(v->point()));
                } else {
                  fprintf(stderr, "Duplicate vertex in convex decomposition\n");
                }
              }

              void visit(Nef::Halffacet_const_handle) {}
              void visit(Nef::SFace_const_handle) {}
              void visit(Nef::Halfedge_const_handle) {}
              void visit(Nef::SHalfedge_const_handle) {}
              void visit(Nef::SHalfloop_const_handle) {}
            };
            VertexCollector collector;
            collector.points.reserve(decomposed_nef.number_of_vertices());
            collector.vertices_seen.reserve(decomposed_nef.number_of_vertices());

            // the first volume is the outer volume, which ignored in the decomposition
            Nef::Volume_const_iterator ci = ++decomposed_nef.volumes_begin();
            for (; ci != decomposed_nef.volumes_end(); ++ci) {
              if (ci->mark()) {
                collector.clear();
                Nef::SNC_const_decorator(decomposed_nef).visit_shell_objects(ci->shells_begin(), collector);
                part_points.push_back(collector.points);
              }
            }
            PRINTDB("Minkowski: decomposed into %d convex parts", part_points.size());
            t.stop();
            PRINTDB("Minkowski: decomposition took %f s", t.time());
          }
        }
        
        return part_points;
      });
      
      auto combineParts = [&](const Hull_Points &points0, const Hull_Points &points1) -> std::shared_ptr<const ManifoldGeometry> {
        CGAL::Timer t;

        t.start();
        std::vector<Hull_Point> minkowski_points;

        auto np0 = points0.size();
        auto np1 = points1.size();
        minkowski_points.resize(np0 * np1);
        for (size_t i = 0; i < points0.size(); ++i) {
          auto &p0i = points0[i];
          auto offset = np1 * i;
          for (size_t j = 0; j < points1.size(); ++j) {
            minkowski_points[offset + j] = p0i + points1[j];
          }
        }

        if (minkowski_points.size() <= 3) {
          t.stop();
          return std::make_shared<const ManifoldGeometry>();
        }

        t.stop();
        PRINTDB("Minkowski: Point cloud creation (%d ⨉ %d -> %d) took %f ms", points0.size() % points1.size() % minkowski_points.size() % (t.time() * 1000));
        t.reset();

        t.start();

        auto hull = manifold::Manifold::Hull(minkowski_points);
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
      Geometry::Geometries fake_children;
      for (const auto& part : result_parts) {
        fake_children.push_back(std::make_pair(std::shared_ptr<const AbstractNode>(),
                                                part));
      }
      auto N = ManifoldUtils::applyOperator3DManifold(fake_children, OpenSCADOperator::UNION);

      // FIXME: This should really never throw.
      // Assert once we figured out what went wrong with issue #1069?
      if (!N) throw 0;
      t.stop();
      PRINTDB("Minkowski: Union done: %f s", t.time());
      t.reset();

      N->toOriginal();
      operands[0] = N;
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
  return ManifoldUtils::applyOperator3DManifold(children, OpenSCADOperator::MINKOWSKI);
}

}  // namespace ManifoldUtils

#endif // ENABLE_MANIFOLD
