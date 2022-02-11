// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
/*
  OpenSCAD CGAL Bug Repro Tool

  When OpenSCAD crashes in CGAL routines, this tool helps reproduce and isolate
  the crash with simpler inputs.

  - Step 1: build the tool:

    mkdir -p buildDebug cd buildDebug
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_CGAL_BUG_REPRO_TOOL=1
    make VERBOSE=1 cgal-bug-repro
    # or just make -j to also build OpenSCAD

  - Step 2: run the hanging or crashing model in OpenSCAD w/ the flags
    --enable=fast-csg --enable=fast-csg-debug-corefinement

  - Step 3: find the .off files written with the geometry of the left-hand-side
    and right-hand-side operands of the last operation, and rename them to
    something simpler, like lhs.off and rhs.off. Keep note of the type of
    operation (union, difference, intersection...)

  - Step 4: run the repro too on the files to confirm there's a likely bug in
    CGAL:

      ./cgal-bug-repro epeck nef union lhs.off rhs.off

  - Step 5: file a bug with the repro instructions (model included) at
    https://github.com/CGAL/cgal/issues


  Note: explicit compilation instructions that may or may not work:

    g++ \
      -O2 \
      -stdlib=libc++ -std=c++1y \
      -I../CGAL-5.2.x/build/include \
      -lgmp -lmpfr \
      cgal-bug-repro.cc \
      -o cgal-bug-repro
*/

#include <boost/algorithm/string.hpp>
// #include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <fstream>

#include <CGAL/boost/graph/helpers.h>
#include <CGAL/boost/graph/iterator.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Extended_cartesian.h>
#include <CGAL/Gmpq.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/stitch_borders.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Surface_mesh.h>

// #include "Filtered_rational_kernel.h"

enum KernelType { UnknownKernelType, Epeck, CartesianGmpq, FilteredRational };
enum Mode { UnknownMode, Corefinement, Nef };
enum Operation { UnknownOperation, Union, Difference, Intersection };

namespace PMP = CGAL::Polygon_mesh_processing;
namespace NP = CGAL::parameters;

template <typename Mesh>
void printMeshStats(const std::string &file, const Mesh &mesh)
{
	auto closed = CGAL::is_closed(mesh);
	std::cout << file.c_str() << ":\n\t" << mesh.number_of_faces() << " facets,\n\t"
						<< mesh.number_of_vertices() << " vertices,\n\t"
						<< (closed ? "closed" : "NOT CLOSED!") << "\n";
}

bool getBoolEnv(const char *name, bool defaultValue = false) {
  auto cstr = getenv(name);
  if (!cstr) return defaultValue;
  std::string str(cstr);
  boost::algorithm::to_lower(str);
  if (str == "1" || str == "on" || str == "true") {
    std::cout << name << " is true\n";
    return true;
  }
  if (str == "0" || str == "off" || str == "false") {
    std::cout << name << " is false\n";
    return false;
  }
  std::cout << "Unexpected value for boolean env var " << name << ": " << cstr << "\n";
  exit(EXIT_FAILURE);
}

template <typename Kernel>
int run(Mode mode, Operation op, const std::string &lhsFile, const std::string &rhsFile)
{
	typedef CGAL::Polyhedron_3<Kernel> Polyhedron;
	typedef CGAL::Nef_polyhedron_3<Kernel> Nef;
	typedef typename Kernel::Point_3 Point_3;
	typedef CGAL::Surface_mesh<Point_3> SurfaceMesh;

	if (mode == Mode::Nef) {
		Polyhedron lhs;
		Polyhedron rhs;

		std::ifstream(lhsFile) >> lhs;
		std::ifstream(rhsFile) >> rhs;

		std::cout << lhsFile << ": " << lhs.size_of_facets() << " facets\n";
		std::cout << rhsFile << ": " << rhs.size_of_facets() << " facets\n";

		std::cout << "Creating nefs...\n";
		Nef lhsNef(lhs);
		Nef rhsNef(rhs);
		Nef resultNef;
		std::cout << "Running Nef " << op << " of " << lhsFile << " and " << rhsFile << "...\n";
		if (op == Union)
			resultNef = lhsNef + rhsNef;
		else if (op == Difference)
			resultNef = lhsNef - rhsNef;
		else if (op == Intersection)
			resultNef = lhsNef * rhsNef;
		else
			assert(false);
		std::cout << "Result: " << resultNef.number_of_facets() << " faces" << std::endl;
	}
	else if (mode == Mode::Corefinement) {
		SurfaceMesh lhs;
		SurfaceMesh rhs;

		std::ifstream(lhsFile) >> lhs;
		std::ifstream(rhsFile) >> rhs;

		printMeshStats(lhsFile, lhs);
		printMeshStats(rhsFile, rhs);

    auto runOp = [&](auto throwOnSelfIntersection) {
      SurfaceMesh result;

      auto lhsParams = CGAL::parameters::throw_on_self_intersection(throwOnSelfIntersection);
      auto rhsParams = CGAL::parameters::throw_on_self_intersection(throwOnSelfIntersection);
      auto resultParams = CGAL::parameters::throw_on_self_intersection(throwOnSelfIntersection);

      // auto lhsParams = CGAL::parameters::all_default();
      // auto rhsParams = CGAL::parameters::all_default();
      // auto resultParams = CGAL::parameters::all_default();

      std::cout << "Running Surface_mesh Corefinement " << op << " of " << lhsFile << " and "
                << rhsFile << "...\n";
      if (op == Union)
        PMP::corefine_and_compute_union(lhs, rhs, result, lhsParams,
                                                                  rhsParams, resultParams);
      else if (op == Difference)
        PMP::corefine_and_compute_difference(lhs, rhs, result, lhsParams,
                                                                      rhsParams, resultParams);
      else if (op == Intersection)
        PMP::corefine_and_compute_intersection(lhs, rhs, result, lhsParams,
                                                                        rhsParams, resultParams);
      else
        assert(false);
      std::cout << "Result: " << result.number_of_faces() << " faces" << std::endl;
    };

    auto repairSelfIntersections = getBoolEnv("REPAIR_SELF_INTERSECTIONS", false);
    try {
      runOp(/* throwOnSelfIntersection= */ repairSelfIntersections);
    } catch (PMP::Corefinement::Self_intersection_exception &e) {
      std::cerr << "Self_intersection_exception: " << e.what() << "\n";
      std::cerr << "Trying to repair the operands and start again.\n";

      auto repair = [](SurfaceMesh &mesh) {
        // PMP::stitch_borders(mesh);

        // // Fix manifoldness by splitting non-manifold vertices
        // typedef typename boost::graph_traits<SurfaceMesh>::vertex_descriptor                 vertex_descriptor;
        // typedef typename boost::graph_traits<SurfaceMesh>::halfedge_descriptor               halfedge_descriptor;
        // std::vector<std::vector<vertex_descriptor> > duplicated_vertices;
        // std::size_t new_vertices_nb = PMP::duplicate_non_manifold_vertices(mesh,
        //                                                                   NP::output_iterator(
        //                                                                     std::back_inserter(duplicated_vertices)));
        // std::cout << new_vertices_nb << " vertices have been added to fix mesh manifoldness" << std::endl;
        // for(std::size_t i=0; i<duplicated_vertices.size(); ++i)
        // {
        //   std::cout << "Non-manifold vertex " << duplicated_vertices[i].front() << " was fixed by creating";
        //   for(std::size_t j=1; j<duplicated_vertices[i].size(); ++j)
        //     std::cout << " " << duplicated_vertices[i][j];
        //   std::cout << std::endl;
        // }
        PMP::experimental::remove_self_intersections(mesh);
        // PMP::repair_polygon_soup(
        //     points, polygons); //, CGAL::parameters::geom_traits(Array_traits()));

        // auto out = std::make_shared<CGAL::Polyhedron_3<K>>();
        // PMP::polygon_soup_to_polygon_mesh(points, polygons, *out);
      };
      repair(lhs);
      repair(rhs);

      try {
        runOp(/* throwOnSelfIntersection= */ true);
      } catch (PMP::Corefinement::Self_intersection_exception &e) {
        std::cerr << "Self_intersection_exception again! Will now try without throwing, might hang: " << e.what() << "\n";
        runOp(/* throwOnSelfIntersection= */ false);
      }
    }
	}
	else {
		assert(false);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	std::map<std::string, Mode> modes = {
			{"coref", Corefinement},
			{"nef", Nef},
	};
	std::map<std::string, KernelType> kernels = {
			{"epeck", Epeck},
			{"cartesian_gmpq", CartesianGmpq},
      {"filtered_rational", FilteredRational},
	};
	std::map<std::string, Operation> ops = {
			{"union", Union},
			{"difference", Difference},
			{"intersection", Intersection},
	};

	auto valuesOf = [=](auto map, auto sep) {
		std::vector<std::string> values;
		boost::copy(map | boost::adaptors::map_keys, std::back_inserter(values));
		return boost::algorithm::join(values, sep);
	};
	auto usage = [=]() {
		std::cerr << "\nOpenSCAD's CGAL Bug Reproduction Case utility\n\n";
		std::cerr << "Please run the problematic (crashing or hanging) file in OpenSCAD with "
								 "--enable=fast-csg --enable=fast-csg-debug-corefinement and look out for the most "
								 "recent *lhs.off & *rhs.off files created, then use them to run this tool:\n\n";
		std::cerr << "Usage: (" << valuesOf(kernels, "|").c_str() << ") ("
							<< valuesOf(modes, "|").c_str() << ") (" << valuesOf(ops, "|").c_str()
							<< ") lhs.off rhs.off\n";
	};
	auto parseArg = [=](auto name, auto map, auto arg) {
		auto it = map.find(arg);
		if (it == map.end()) {
			std::cerr << "Invalid " << name << ": " << arg << " (expected: any of "
								<< valuesOf(map, ", ").c_str() << ")\n";
			usage();
			exit(EXIT_FAILURE);
		}
		return it->second;
	};

	if (argc != 6) {
		usage();
		return EXIT_FAILURE;
	}
	auto kernel = parseArg("kernel", kernels, argv[1]);
	auto mode = parseArg("mode", modes, argv[2]);
	auto op = parseArg("op", ops, argv[3]);
	std::string lhsFile(argv[4]);
	std::string rhsFile(argv[5]);

	try {
		if (kernel == Epeck) {
			return run<CGAL::Epeck>(mode, op, lhsFile, rhsFile);
		}

    // typedef CGAL::Simple_cartesian<CGAL::Interval_nt<false> >                    FRK_IA;
    // typedef CGAL::Simple_cartesian<CGAL::internal::Exact_field_selector<double>::Type> FRK_EA;
    // typedef CGAL::Filtered_rational_kernel<FRK_IA, FRK_EA>                       Epeck2;

		// if (kernel == FilteredRational) {
		// 	return run<Epeck2>(mode, op, lhsFile, rhsFile);
		// }
    // Note: remove_self_intersections doesn't compile with this one:
		// if (kernel == CartesianGmpq) {
		// 	return run<CGAL::Cartesian<CGAL::Gmpq>>(mode, op, lhsFile, rhsFile);
		// }
		return EXIT_SUCCESS;

	} catch (CGAL::Assertion_exception &e) {
		std::cerr << "Assertion_exception: " << e.what() << "\n";
		return EXIT_FAILURE;
	} catch (CGAL::Failure_exception &e) {
		std::cerr << "Failure_exception: " << e.what() << "\n";
		return EXIT_FAILURE;
	} catch (PMP::Corefinement::Self_intersection_exception &e) {
		std::cerr << "Self_intersection_exception: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}
