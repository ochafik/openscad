# Triggers code that will update CGAL_Nef_polyhedron3 in place (in +=, -= and
# *= operators), which should might only make a difference for intersections for
# now.
#
# qmake VARIANT=inplace_nef_ops ...

QMAKE_CXXFLAGS += -DCGAL_NEF_VISUAL_HULL
QMAKE_CXXFLAGS += -DINPLACE_NEF_OPS
