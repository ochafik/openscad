#pragma once
#undef DIFFERENCE //#defined in winuser.h

enum class OpenSCADOperator {
	UNION,
	INTERSECTION,
	DIFFERENCE,
	MINKOWSKI,
	HULL,
	RESIZE
};

inline const char* getOperatorName(OpenSCADOperator op) {
  switch (op) {
	  case OpenSCADOperator::UNION:
      return "UNION";
	  case OpenSCADOperator::INTERSECTION:
      return "INTERSECTION";
	  case OpenSCADOperator::DIFFERENCE:
      return "DIFFERENCE";
	  case OpenSCADOperator::MINKOWSKI:
      return "MINKOWSKI";
	  case OpenSCADOperator::HULL:
      return "HULL";
	  case OpenSCADOperator::RESIZE:
      return "RESIZE";
    default:
      assert(false);
      return "?";
  }
}
