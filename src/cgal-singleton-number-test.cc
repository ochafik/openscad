/*
g++ -std=c++14 \
	-DDEBUG=1 -O0 -g \
  -DCGAL_USE_GMPXX=1 -lgmp -lmpfr \
	-o cgal-singleton-number-test src/cgal-singleton-number-test.cc && \
  ./cgal-singleton-number-test
*/
#include <CGAL/Gmpq.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/Filtered_kernel.h>
#include <CGAL/Cartesian.h>
#include "cgal-singleton-number.h"

#include <fstream>

extern std::string example1; // see bottom of file
extern std::string example2;

typedef SingletonNumber<CGAL::Gmpq> FT;
typedef CGAL::Cartesian<FT> K;
// typedef CGAL::Cartesian<CGAL::Gmpq> K;
// typedef CGAL::Simple_cartesian<double> K;
// typedef CGAL::Epeck K;

// TODO(ochafik): Experiment with this:
// typedef CGAL::Filtered_kernel<CGAL::Simple_cartesian<CGAL::Lazy_exact_nt<FT>>> K;

typedef CGAL::Surface_mesh<K::Point_3> Mesh;
typedef CGAL::Polyhedron_3<K> Poly;
typedef CGAL::Nef_polyhedron_3<K> Nef;

int main()
{  
  try {
#ifdef TEST
    bool b;
    FT x(1), y(2), z;

    auto expect = [&](auto actual, auto expected) {
      if ((actual != expected) || !(actual == expected)) {
        std::cout << "actual == " << CGAL::to_double(actual) << " (!= expected " << expected << ")\n";
        assert(z == expected);
      }
    };
    auto expectBool = [&](bool actual, bool expected) {
      if (actual != expected) {
        std::cout << "actual == " << (actual ? "true" : "false") << " (!= expected " << (expected ? "true" : "false") << ")\n";
        assert(z == expected);
      }
    };

    //
    // Plus
    //
    expect(FT(0) + 1, 1);
    expect(2 + FT(0), 2);
    expect(FT(10) + 0, 10);
    expect(0 + FT(11), 11);
    
    expect(FT(2) + 3, 5);
    expect(2 + FT(4), 6);
    expect(FT(2) + FT(5), 7);

    //
    // Minus
    //

    expect(FT(0) - 1, -1);
    expect(2 - FT(0), 2);
    expect(FT(20) - 0, 20);
    expect(0 - FT(20), -20);
    
    expect(FT(2) + 3, 5);
    expect(2 + FT(4), 6);
    expect(FT(2) + FT(5), 7);
    
    // Zero equality an unary minus

    expect(FT(0), 0);
    expect(FT(-0), 0);
    expect(FT(0), FT(-0));

    expect(FT(1) - FT(1), 0);
    expect(FT(0) - FT(1), FT(-1));
    expect(-FT(2), -2);
    expect(-FT(0), 0);

    //
    // Multiply
    //
    expect(FT(0) * 2, 0);
    expect(2 * FT(0), 0);

    expect(0 * FT(2), 0);
    expect(FT(2) * 0, 0);
    
    expect(FT(2) * 3, 6);
    expect(2 * FT(4), 8);
    expect(FT(2) * FT(5), 10);

    //
    // Divide
    //
    expect(FT(0) / 2, 0);
    expect(0 / FT(2), 0);
    expect(0 / FT(0), 0);

    expect(4 / FT(2), 2);
    expect(FT(9) / 3, 3);
    expect(FT(12) / FT(3), 4);

    expect(FT(1) / 3, CGAL::Gmpq(1, 3));
    expect(FT(1) / FT(3), CGAL::Gmpq(1, 3));
    expect(1 / FT(3), CGAL::Gmpq(1, 3));

    // CGAL::Gmpq(1, 0) / CGAL::Gmpq(0, 1);
    // FT(1) / FT(0); 

    //
    // Less
    //
    expectBool(FT(0) < 2, true);
    expectBool(FT(0) < -1, false);
    expectBool(FT(0) < 0, false);
    expectBool(FT(2) < 3, true);
    expectBool(FT(2) < 0, false);
    expectBool(FT(-1) < 0, true);
    expectBool(FT(3) < 3, false);
    expectBool(FT(10) < FT(11), true);
    expectBool(FT(11) < FT(10), false);

    //
    // More
    //
    expectBool(FT(0) > 2, false);
    expectBool(FT(0) > -1, true);
    expectBool(FT(0) > 0, false);
    expectBool(FT(3) > 2, true);
    expectBool(FT(2) > 0, true);
    expectBool(FT(2) > 3, false);
    expectBool(FT(3) > 3, false);
    expectBool(FT(11) > FT(10), true);
    expectBool(FT(10) > FT(11), false);

    
    // std::cout << SingletonNumber<CGAL::Gmpq>::cache.values_.size() << " values\n";
    // for (auto &value : SingletonNumber<CGAL::Gmpq>::cache.values_) {
    //   std::cout << value << "\n";
    // }
#else

    // Nef nef1, nef2;
    Poly poly1, poly2;
    Mesh mesh1, mesh2;

    {
      std::stringstream ss;
      ss << example1;
      ss >> poly1;
      ss >> mesh1;
      assert(CGAL::is_closed(mesh1)); // OK
      assert(CGAL::is_valid_halfedge_graph(mesh1, /* verb */ false)); // OK
    }
    {
      std::stringstream ss;
      ss << example2;
      ss >> poly2;
      ss >> mesh2;
      assert(CGAL::is_closed(mesh2)); // OK
      assert(CGAL::is_valid_halfedge_graph(mesh2, /* verb */ false)); // OK
    }

    CGAL::Polygon_mesh_processing::triangulate_faces(mesh1);
    CGAL::Polygon_mesh_processing::triangulate_faces(mesh2);


    std::cout << "corefinement: " << (CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(mesh1, mesh2, mesh1) ? "true" : "false") << "\n";
    
    Nef nef1(poly1), nef2(poly2);
    nef1 += nef2;
    
#endif
  } catch (CGAL::Failure_exception& e) {
    std::cerr << "CGAL error: " << e.what() << "\n";
  }

  SingletonNumber<CGAL::Gmpq>::values.printStats();
#ifndef LOCAL_SINGLETON_OPS_CACHE
  SingletonNumber<CGAL::Gmpq>::cache.printStats();
#endif
  
  return 0;
}


// `sphere(1, $fn=8);` in OpenSCAD
std::string example1(
"OFF\n\
32 60 0\n\
0.270598 0.270598 0.92388\n\
0 0.382683 0.92388\n\
-0.270598 0.270598 0.92388\n\
-0.382683 0 0.92388\n\
-0.270598 -0.270598 0.92388\n\
0 -0.382683 0.92388\n\
0.270598 -0.270598 0.92388\n\
0.382683 0 0.92388\n\
0.653281 0.653281 0.382683\n\
0 0.92388 0.382683\n\
-0.653281 0.653281 0.382683\n\
-0.92388 0 0.382683\n\
-0.653281 -0.653281 0.382683\n\
0 -0.92388 0.382683\n\
0.653281 -0.653281 0.382683\n\
0.92388 0 0.382683\n\
0.653281 0.653281 -0.382683\n\
0 0.92388 -0.382683\n\
-0.653281 0.653281 -0.382683\n\
-0.92388 0 -0.382683\n\
-0.653281 -0.653281 -0.382683\n\
0 -0.92388 -0.382683\n\
0.653281 -0.653281 -0.382683\n\
0.382683 0 -0.92388\n\
0.92388 0 -0.382683\n\
0.270598 0.270598 -0.92388\n\
0 0.382683 -0.92388\n\
-0.270598 0.270598 -0.92388\n\
-0.382683 0 -0.92388\n\
-0.270598 -0.270598 -0.92388\n\
0 -0.382683 -0.92388\n\
0.270598 -0.270598 -0.92388\n\
3  7 15 8\n\
3  7 8 0\n\
3  0 8 9\n\
3  0 9 1\n\
3  1 9 10\n\
3  1 10 2\n\
3  2 10 11\n\
3  2 11 3\n\
3  3 11 12\n\
3  3 12 4\n\
3  4 12 13\n\
3  4 13 5\n\
3  5 13 14\n\
3  5 14 6\n\
3  6 14 15\n\
3  6 15 7\n\
3  15 24 16\n\
3  15 16 8\n\
3  8 16 17\n\
3  8 17 9\n\
3  9 17 18\n\
3  9 18 10\n\
3  10 18 19\n\
3  10 19 11\n\
3  11 19 20\n\
3  11 20 12\n\
3  12 20 21\n\
3  12 21 13\n\
3  13 21 22\n\
3  13 22 14\n\
3  14 22 24\n\
3  14 24 15\n\
3  24 23 25\n\
3  24 25 16\n\
3  16 25 26\n\
3  16 26 17\n\
3  17 26 27\n\
3  17 27 18\n\
3  18 27 28\n\
3  18 28 19\n\
3  19 28 29\n\
3  19 29 20\n\
3  20 29 30\n\
3  20 30 21\n\
3  21 30 31\n\
3  21 31 22\n\
3  22 31 23\n\
3  22 23 24\n\
3  0 1 7\n\
3  5 1 3\n\
3  3 1 2\n\
3  5 3 4\n\
3  7 1 5\n\
3  7 5 6\n\
3  30 29 28\n\
3  26 30 28\n\
3  27 26 28\n\
3  25 23 26\n\
3  23 30 26\n\
3  31 30 23\n\
");

// Derived from `sphere(1, $fn=3);` in OpenSCAD (edited to have integer coordinates, but same issue as the original + same general shape)
std::string example2(
"OFF\n\
6 8 0\n\
-3.0 6.0 7.0\n\
-3.0 -6.0 7.0\n\
7.0 0 -7.0\n\
7.0 0 7.0\n\
-3.0 6.0 -7.0\n\
-3.0 -6.0 -7.0\n\
3  1 3 0\n\
3  3 2 4\n\
3  3 4 0\n\
3  0 4 5\n\
3  0 5 1\n\
3  1 5 2\n\
3  1 2 3\n\
3  2 5 4\n\
");
