// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM

#ifdef ENABLE_CGAL

#include "cgal.h"
#include "cgalutils.h"
#include "PolySet.h"
#include "printutils.h"
#include "CGALHybridPolyhedron.h"
#include "node.h"
#include "manifoldutils.h"
#include "ManifoldGeometry.h"

#include <CGAL/convex_hull_3.h>

#include "memory.h"

#include <map>
#include <queue>
#include <unordered_set>

namespace ManifoldUtils {


/*!
   children cannot contain nullptr objects
 */
shared_ptr<const Geometry> applyMinkowskiManifold(const Geometry::Geometries& children)
{
  using Hull_kernel = CGAL::Epick;
  using Hull_Polyhedron = CGAL::Polyhedron_3<Hull_kernel>;
  using Hull_Mesh = CGAL::Surface_mesh<CGAL::Point_3<Hull_kernel>>;
  using Nef_kernel = CGAL_Kernel3;
  using Polyhedron = CGAL_Polyhedron;
  using Nef = CGAL_Nef_polyhedron3;

  auto polyhedronFromGeometry = [](const shared_ptr<const Geometry>& geom, bool *pIsConvexOut) -> shared_ptr<Polyhedron> 
  {
    auto ps = dynamic_pointer_cast<const PolySet>(geom);
    if (ps) {
      auto poly = make_shared<Polyhedron>();
      CGALUtils::createPolyhedronFromPolySet(*ps, *poly);
      if (pIsConvexOut) *pIsConvexOut = ps->is_convex();
      return poly;
    } else {
      if (auto mani = dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
        auto poly = mani->toPolyhedron<Polyhedron>();
        if (pIsConvexOut) *pIsConvexOut = CGALUtils::is_weakly_convex(*poly);
        return poly;
      } else throw 0;
    }
    throw 0;
  };
  
  CGAL::Timer t, t_tot;
  assert(children.size() >= 2);
  auto it = children.begin();
  t_tot.start();
  shared_ptr<const Geometry> operands[2] = {it->second, shared_ptr<const Geometry>()};
  try {
    while (++it != children.end()) {
      operands[1] = it->second;

      std::list<shared_ptr<Polyhedron>> P[2];
      std::list<shared_ptr<Hull_Polyhedron>> result_parts;

      for (size_t i = 0; i < 2; ++i) {
        bool is_convex;
        auto poly = polyhedronFromGeometry(operands[i], &is_convex);
        if (!poly) throw 0;
        if (poly->empty()) {
          throw 0;
        }

        if (is_convex) {
          PRINTDB("Minkowski: child %d is convex and %s", i % (dynamic_pointer_cast<const PolySet>(operands[i])?"PolySet":"Hybrid"));
          P[i].push_back(poly);
        } else {
          PRINTDB("Minkowski: child %d is nonconvex, decomposing...", i);
          auto decomposed_nef = make_shared<Nef>(*poly);

          t.start();
          CGAL::convex_decomposition_3(*decomposed_nef);

          // the first volume is the outer volume, which ignored in the decomposition
          Nef::Volume_const_iterator ci = ++decomposed_nef->volumes_begin();
          for (; ci != decomposed_nef->volumes_end(); ++ci) {
            if (ci->mark()) {
              auto poly = make_shared<Polyhedron>();
              decomposed_nef->convert_inner_shell_to_polyhedron(ci->shells_begin(), *poly);
              P[i].push_back(poly);
            }
          }

          PRINTDB("Minkowski: decomposed into %d convex parts", P[i].size());
          t.stop();
          PRINTDB("Minkowski: decomposition took %f s", t.time());
        }
      }

      std::vector<Hull_kernel::Point_3> points[2];
      std::vector<Hull_kernel::Point_3> minkowski_points;

      CGAL::Cartesian_converter<Nef_kernel, Hull_kernel> conv;

      for (size_t i = 0; i < P[0].size(); ++i) {
        for (size_t j = 0; j < P[1].size(); ++j) {
          t.start();
          points[0].clear();
          points[1].clear();

          for (int k = 0; k < 2; ++k) {
            auto it = P[k].begin();
            std::advance(it, k == 0?i:j);

            auto& poly = *it;
            points[k].reserve(poly->size_of_vertices());

            for (auto pi = poly->vertices_begin(); pi != poly->vertices_end(); ++pi) {
              const Polyhedron::Point_3 &p = pi->point();
              points[k].push_back(conv(p));
            }
          }

          minkowski_points.clear();
          minkowski_points.reserve(points[0].size() * points[1].size());
          for (size_t i = 0; i < points[0].size(); ++i) {
            for (size_t j = 0; j < points[1].size(); ++j) {
              minkowski_points.push_back(points[0][i] + (points[1][j] - CGAL::ORIGIN));
            }
          }

          if (minkowski_points.size() <= 3) {
            t.stop();
            continue;
          }

          auto result = make_shared<Hull_Polyhedron>();
          t.stop();
          PRINTDB("Minkowski: Point cloud creation (%d ⨉ %d -> %d) took %f ms", points[0].size() % points[1].size() % minkowski_points.size() % (t.time() * 1000));
          t.reset();

          t.start();

          CGAL::convex_hull_3(minkowski_points.begin(), minkowski_points.end(), *result);

          std::vector<Hull_kernel::Point_3> strict_points;
          strict_points.reserve(minkowski_points.size());

          for (auto i = result->vertices_begin(); i != result->vertices_end(); ++i) {
            auto& p = i->point();

            auto h = i->halfedge();
            auto e = h;
            bool collinear = false;
            bool coplanar = true;

            do {
              auto& q = h->opposite()->vertex()->point();
              if (coplanar && !CGAL::coplanar(p, q,
                                              h->next_on_vertex()->opposite()->vertex()->point(),
                                              h->next_on_vertex()->next_on_vertex()->opposite()->vertex()->point())) {
                coplanar = false;
              }


              for (auto j = h->next_on_vertex();
                   j != h && !collinear && !coplanar;
                   j = j->next_on_vertex()) {

                auto& r = j->opposite()->vertex()->point();
                if (CGAL::collinear(p, q, r)) {
                  collinear = true;
                }
              }

              h = h->next_on_vertex();
            } while (h != e && !collinear);

            if (!collinear && !coplanar) strict_points.push_back(p);
          }

          result->clear();
          CGAL::convex_hull_3(strict_points.begin(), strict_points.end(), *result);


          t.stop();
          PRINTDB("Minkowski: Computing convex hull took %f s", t.time());
          t.reset();

          result_parts.push_back(result);
        }
      }

      if (it != std::next(children.begin())) operands[0].reset();

      auto partToGeom = [&](auto& poly) -> shared_ptr<const Geometry> {
        auto mesh = make_shared<Hull_Mesh>();
        CGAL::copy_face_graph(*poly, *mesh);
        CGALUtils::triangulateFaces(*mesh);
#if 1
        return ManifoldUtils::createMutableManifoldFromSurfaceMesh(*mesh);
#else
        PolySet ps(3);
        CGALUtils::createPolySetFromMesh(*mesh, ps);
        return ManifoldUtils::createMutableManifoldFromPolySet(ps);
#endif
      };

      if (result_parts.size() == 1) {
        operands[0] = partToGeom(*result_parts.begin());
      } else if (!result_parts.empty()) {
        t.start();
        PRINTDB("Minkowski: Computing union of %d parts", result_parts.size());
        Geometry::Geometries fake_children;
        for (const auto& part : result_parts) {
          fake_children.push_back(std::make_pair(std::shared_ptr<const AbstractNode>(),
                                                 partToGeom(part)));
        }
        auto N = ManifoldUtils::applyOperator3DManifold(fake_children, OpenSCADOperator::UNION);
        
        // FIXME: This should really never throw.
        // Assert once we figured out what went wrong with issue #1069?
        if (!N) throw 0;
        t.stop();
        PRINTDB("Minkowski: Union done: %f s", t.time());
        t.reset();
        operands[0] = N;
      } else {
        operands[0] = make_shared<const ManifoldGeometry>();
      }
    }

    t_tot.stop();
    PRINTDB("Minkowski: Total execution time %f s", t_tot.time());
    t_tot.reset();
    return operands[0];
  } catch (const std::exception& e) {
    LOG(message_group::Warning, Location::NONE, "",
        "[manifold] Minkowski failed with error, falling back to Nef operation: %1$s\n", e.what());

    auto N = applyOperator3DManifold(children, OpenSCADOperator::MINKOWSKI);
    return N;
  }
}

}  // namespace ManifoldUtils


#endif // ENABLE_CGAL








