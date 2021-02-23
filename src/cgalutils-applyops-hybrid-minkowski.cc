// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM

#ifdef ENABLE_CGAL

#include "cgal.h"

#ifdef FAST_CSG_AVAILABLE

#include "cgalutils.h"
#include "polyset.h"
#include "printutils.h"
#include "CGALHybridPolyhedron.h"

#pragma push_macro("NDEBUG")
#undef NDEBUG
#include <CGAL/convex_hull_3.h>
#pragma pop_macro("NDEBUG")

#include <queue>
#include <unordered_set>

namespace CGALUtils {

	/*!
		children cannot contain nullptr objects
	*/
	shared_ptr<const Geometry> applyMinkowskiHybrid(const Geometry::Geometries &children)
	{
		CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
		CGAL::Timer t,t_tot;
		assert(children.size() >= 2);
		Geometry::Geometries::const_iterator it = children.begin();
		t_tot.start();
		shared_ptr<const Geometry> operands[2] = {it->second, shared_ptr<const Geometry>()};
		try {
			while (++it != children.end()) {
				operands[1] = it->second;

				// typedef CGAL::Epick Hull_kernel;
				typedef CGAL_HybridKernel3 Hull_kernel;

				std::list<shared_ptr<CGALHybridPolyhedron>> P[2];
        // shared_ptr<CGALHybridPolyhedron> unionResult;
				std::list<shared_ptr<CGALHybridPolyhedron>> result_parts;

				for (size_t i = 0; i < 2; ++i) {
					if (!operands[i]) throw 0;
					auto ps = dynamic_pointer_cast<const PolySet>(operands[i]);
					auto hybrid = CGALUtils::createHybridPolyhedronFromGeometry(*operands[i]);
					if (!hybrid) throw 0;

          // TODO(ochafik): implement is_weakly_convex for Surface_mesh instead of force-converting
          // what could be a Surface_mesh to a polyhedron!
					if ((ps && ps->is_convex()) ||
              is_weakly_convex(hybrid->convert<CGALHybridPolyhedron::polyhedron_t>())) {
						PRINTDB("Minkowski: child %d is convex and %s",i % (ps?"PolySet":"Nef"));
						P[i].push_back(hybrid);
					} else {
						PRINTDB("Minkowski: child %d is nonconvex, decomposing...",i);
						auto & decomposed_nef = hybrid->convert<CGALHybridPolyhedron::nef_polyhedron_t>();

						t.start();
						CGAL::convex_decomposition_3(decomposed_nef);

						// the first volume is the outer volume, which ignored in the decomposition
						CGALHybridPolyhedron::nef_polyhedron_t::Volume_const_iterator ci = ++decomposed_nef.volumes_begin();
						for(; ci != decomposed_nef.volumes_end(); ++ci) {
							if(ci->mark()) {
								auto poly = make_shared<CGALHybridPolyhedron::polyhedron_t>();
								decomposed_nef.convert_inner_shell_to_polyhedron(ci->shells_begin(), *poly);

                // TODO(ochafik): Build a Surface_mesh instead?
                P[i].push_back(make_shared<CGALHybridPolyhedron>(poly));
							}
						}


						PRINTDB("Minkowski: decomposed into %d convex parts", P[i].size());
						t.stop();
						PRINTDB("Minkowski: decomposition took %f s", t.time());
					}
				}

				std::vector<Hull_kernel::Point_3> points[2];
				std::vector<Hull_kernel::Point_3> minkowski_points;

				// CGAL::Cartesian_converter<CGAL_HybridKernel3, Hull_kernel> conv;

				for (size_t i = 0; i < P[0].size(); ++i) {
					for (size_t j = 0; j < P[1].size(); ++j) {
						t.start();
						points[0].clear();
						points[1].clear();

						for (int k = 0; k < 2; ++k) {
							auto it = P[k].begin();
							std::advance(it, k==0?i:j);

							const auto& poly = **it;
							points[k].reserve(poly.numVertices());
							poly.foreachVertexUntilTrue([&](const auto &p) {
								//points[k].push_back(conv(p));
								points[k].push_back(p);
								return false;
							});
						}

						minkowski_points.clear();
						minkowski_points.reserve(points[0].size() * points[1].size());
						for (size_t i = 0; i < points[0].size(); ++i) {
							for (size_t j = 0; j < points[1].size(); ++j) {
								minkowski_points.push_back(points[0][i]+(points[1][j]-CGAL::ORIGIN));
							}
						}

						if (minkowski_points.size() <= 3) {
							t.stop();
							continue;
						}

						auto result = make_shared<CGAL::Polyhedron_3<Hull_kernel>>();
						t.stop();
						PRINTDB("Minkowski: Point cloud creation (%d ⨉ %d -> %d) took %f ms", points[0].size() % points[1].size() % minkowski_points.size() % (t.time()*1000));
						t.reset();

						t.start();

						CGAL::convex_hull_3(minkowski_points.begin(), minkowski_points.end(), *result);

						std::vector<Hull_kernel::Point_3> strict_points;
						strict_points.reserve(minkowski_points.size());

						for (CGAL::Polyhedron_3<Hull_kernel>::Vertex_iterator i = result->vertices_begin(); i != result->vertices_end(); ++i) {
							Hull_kernel::Point_3 const& p = i->point();

							CGAL::Polyhedron_3<Hull_kernel>::Vertex::Halfedge_handle h,e;
							h = i->halfedge();
							e = h;
							bool collinear = false;
							bool coplanar = true;

							do {
								Hull_kernel::Point_3 const& q = h->opposite()->vertex()->point();
								if (coplanar && !CGAL::coplanar(p,q,
																h->next_on_vertex()->opposite()->vertex()->point(),
																h->next_on_vertex()->next_on_vertex()->opposite()->vertex()->point())) {
									coplanar = false;
								}


								for (CGAL::Polyhedron_3<Hull_kernel>::Vertex::Halfedge_handle j = h->next_on_vertex();
									 j != h && !collinear && ! coplanar;
									 j = j->next_on_vertex()) {

									Hull_kernel::Point_3 const& r = j->opposite()->vertex()->point();
									if (CGAL::collinear(p,q,r)) {
										collinear = true;
									}
								}

								h = h->next_on_vertex();
							} while (h != e && !collinear);

							if (!collinear && !coplanar)
								strict_points.push_back(p);
						}

						result->clear();

						CGAL::convex_hull_3(strict_points.begin(), strict_points.end(), *result);

						t.stop();
						PRINTDB("Minkowski: Computing convex hull took %f s", t.time());
						t.reset();

            // auto quantize = false;
            // if (quantize) {
            //   PolySet ps(3,true);
            //   createPolySetFromPolyhedron(*result, ps);
            //   ps.quantizeVertices();
            //   result_parts.push_back(createHybridPolyhedronFromGeometry(ps));
            // } else {
              result_parts.push_back(make_shared<CGALHybridPolyhedron>(result));
            // }
            // auto hybrid = make_shared<CGALHybridPolyhedron>(result);
            // if (!unionResult) unionResult = hybrid;
            // else *unionResult += *hybrid;
					}
				}

        // operands[0] = unionResult;
				if (result_parts.empty()) {
					operands[0] = make_shared<CGALHybridPolyhedron>();
				} else {
					t.start();
					PRINTDB("Minkowski: Computing union of %d parts",result_parts.size());

					std::vector<std::pair<const AbstractNode*, shared_ptr<CGALHybridPolyhedron>>> mutableOperands;
					for (const auto &part : result_parts) {
						mutableOperands.push_back(std::make_pair((const AbstractNode*)nullptr, part));
					}
					auto N = CGALUtils::applyUnion3DHybrid(mutableOperands);
					// FIXME: This should really never throw.
					// Assert once we figured out what went wrong with issue #1069?
					if (!N) throw 0;
					t.stop();
					PRINTDB("Minkowski: Union done: %f s",t.time());
					t.reset();
					operands[0] = N;
				}
			}

			t_tot.stop();
			PRINTDB("Minkowski: Total execution time %f s", t_tot.time());
			t_tot.reset();
			CGAL::set_error_behaviour(old_behaviour);
			return operands[0];
		}
		catch (...) {
			// If anything throws we simply fall back to Nef Minkowski
			PRINTD("Minkowski: Falling back to Nef Minkowski");

			auto N = applyOperator3DHybrid(children, OpenSCADOperator::MINKOWSKI);
			CGAL::set_error_behaviour(old_behaviour);
			return N;
		}
	}
}; // namespace CGALUtils

#endif // FAST_CSG_AVAILABLE

#endif // ENABLE_CGAL








