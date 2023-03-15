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

#ifdef PARALLELIZE_MANIFOLD_MINKOWSKI
#include <thrust/transform.h>
#include <thrust/functional.h>
#include <thrust/execution_policy.h>
#endif

namespace ManifoldUtils {


/*!
   children cannot contain nullptr objects
 */
shared_ptr<const Geometry> applyMinkowskiManifold(const Geometry::Geometries& children)
{
  using Hull_kernel = CGAL::Epick;
  using Hull_Polyhedron = CGAL::Polyhedron_3<Hull_kernel>;
  using Hull_Mesh = CGAL::Surface_mesh<CGAL::Point_3<Hull_kernel>>;
  using Hull_Points = std::vector<Hull_kernel::Point_3>;
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

  CGAL::Cartesian_converter<Nef_kernel, Hull_kernel> conv;
  auto getHullPoints = [&](const Polyhedron &poly) {
    std::vector<Hull_kernel::Point_3> out;
    out.reserve(poly.size_of_vertices());
    for (auto pi = poly.vertices_begin(); pi != poly.vertices_end(); ++pi) {
      out.push_back(conv(pi->point()));
    }
    return std::move(out);
  };

  try {
    // Note: we could parallelize more, e.g. compute all decompositions ahead of time instead of doing them 2 by 2,
    // but this could use substantially more memory.
    while (++it != children.end()) {
      operands[1] = it->second;

      std::list<Hull_Points> part_points[2];

      for (size_t i = 0; i < 2; ++i) {
        bool is_convex;
        auto poly = polyhedronFromGeometry(operands[i], &is_convex);
        if (!poly) throw 0;
        if (poly->empty()) {
          throw 0;
        }

        if (is_convex) {
          PRINTDB("Minkowski: child %d is convex and %s", i % (dynamic_pointer_cast<const PolySet>(operands[i])?"PolySet":"Hybrid"));
          part_points[i].emplace_back(getHullPoints(*poly));
        } else {
          PRINTDB("Minkowski: child %d is nonconvex, decomposing...", i);
          Nef decomposed_nef(*poly);

          t.start();
          CGAL::convex_decomposition_3(decomposed_nef);

          // the first volume is the outer volume, which ignored in the decomposition
          Nef::Volume_const_iterator ci = ++decomposed_nef.volumes_begin();
          for (; ci != decomposed_nef.volumes_end(); ++ci) {
            if (ci->mark()) {
              Polyhedron poly;
              decomposed_nef.convert_inner_shell_to_polyhedron(ci->shells_begin(), poly);
              part_points[i].emplace_back(getHullPoints(poly));
            }
          }

          PRINTDB("Minkowski: decomposed into %d convex parts", part_points[i].size());
          t.stop();
          PRINTDB("Minkowski: decomposition took %f s", t.time());
        }
      }

      std::vector<Hull_kernel::Point_3> minkowski_points;
      
      auto combineParts = [&](const Hull_Points &points0, const Hull_Points &points1) -> shared_ptr<const ManifoldGeometry> {
        CGAL::Timer t;

        t.start();
        std::vector<Hull_kernel::Point_3> minkowski_points;

        minkowski_points.reserve(points0.size() * points1.size());
        for (size_t i = 0; i < points0.size(); ++i) {
          for (size_t j = 0; j < points1.size(); ++j) {
            minkowski_points.push_back(points0[i] + (points1[j] - CGAL::ORIGIN));
          }
        }

        if (minkowski_points.size() <= 3) {
          t.stop();
          return make_shared<const ManifoldGeometry>();
        }

        t.stop();
        PRINTDB("Minkowski: Point cloud creation (%d ⨉ %d -> %d) took %f ms", points0.size() % points1.size() % minkowski_points.size() % (t.time() * 1000));
        t.reset();

        t.start();

        Hull_Mesh mesh;
        CGAL::convex_hull_3(minkowski_points.begin(), minkowski_points.end(), mesh);

        std::vector<Hull_kernel::Point_3> strict_points;
        strict_points.reserve(minkowski_points.size());

        for (auto v : mesh.vertices()) {
          auto &p = mesh.point(v);

          auto h = mesh.halfedge(v);
          auto e = h;
          bool collinear = false;
          bool coplanar = true;

          do {
            auto &q = mesh.point(mesh.target(mesh.opposite(h)));
            if (coplanar && !CGAL::coplanar(p, q,
                                            mesh.point(mesh.target(mesh.next(h))),
                                            mesh.point(mesh.target(mesh.next(mesh.opposite(mesh.next(h))))))) {
              coplanar = false;
            }


            for (auto j = mesh.opposite(mesh.next(h));
                  j != h && !collinear && !coplanar;
                  j = mesh.opposite(mesh.next(j))) {

              auto& r = mesh.point(mesh.target(mesh.opposite(j)));
              if (CGAL::collinear(p, q, r)) {
                collinear = true;
              }
            }

            h = mesh.opposite(mesh.next(h));
          } while (h != e && !collinear);

          if (!collinear && !coplanar) strict_points.push_back(p);
        }

        mesh.clear();
        CGAL::convex_hull_3(strict_points.begin(), strict_points.end(), mesh);

        t.stop();
        PRINTDB("Minkowski: Computing convex hull took %f s", t.time());
        t.reset();

        CGALUtils::triangulateFaces(mesh);
        return ManifoldUtils::createMutableManifoldFromSurfaceMesh(mesh);
      };

#ifdef PARALLELIZE_MANIFOLD_MINKOWSKI
      std::vector<std::pair<const Hull_Points*, const Hull_Points*>> points_pairs;
      for (const auto &points0 : part_points[0]) {
        for (const auto &points1 : part_points[1]) {
          points_pairs.emplace_back(&points0, &points1);
        }
      }
      std::vector<shared_ptr<const ManifoldGeometry>> result_parts(points_pairs.size());

      thrust::transform(points_pairs.begin(), points_pairs.end(), result_parts.begin(), [&](const auto &pair) {
        return combineParts(*pair.first, *pair.second);
      });
#else
      std::vector<shared_ptr<const ManifoldGeometry>> result_parts;
      result_parts.reserve(part_points[0].size() * part_points[1].size());

      for (const auto &points0 : part_points[0]) {
        for (const auto &points1 : part_points[1]) {
          result_parts.push_back(combineParts(points0, points1));
        }
      }
#endif

      if (it != std::next(children.begin())) operands[0].reset();

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

      operands[0] = N;
    }

    t_tot.stop();
    PRINTDB("Minkowski: Total execution time %f s", t_tot.time());
    t_tot.reset();
    return operands[0];
  } catch (const std::exception& e) {
    LOG(message_group::Warning, Location::NONE, "",
        "[manifold] Minkowski failed with error, falling back to Nef operation: %1$s\n", e.what());

    return ManifoldUtils::applyOperator3DManifold(children, OpenSCADOperator::MINKOWSKI);
  } catch (...) {
    LOG(message_group::Warning, Location::NONE, "",
        "[manifold] Minkowski hard-crashed, falling back to Nef operation.");

    return ManifoldUtils::applyOperator3DManifold(children, OpenSCADOperator::MINKOWSKI);
  }
}

}  // namespace ManifoldUtils


#endif // ENABLE_CGAL








