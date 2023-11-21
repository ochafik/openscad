#pragma once

#include "NodeVisitor.h"
#include "enums.h"
#include "memory.h"
#include "Geometry.h"

#include <utility>
#include <list>
#include <vector>
#include <map>

class CGAL_Nef_polyhedron;
class Polygon2d;
class Tree;

class ResultObject
{
public:
  // Default constructor with nullptr can be used to represent empty geometry,
  // for example union() with no children, etc.
  ResultObject() : is_const(true) {}
  ResultObject(const Geometry *g) : is_const(true), const_pointer(g) {}
  ResultObject(shared_ptr<const Geometry> g) : is_const(true), const_pointer(std::move(g)) {}
  ResultObject(Geometry *g) : is_const(false), pointer(g) {}
  ResultObject(shared_ptr<Geometry>& g) : is_const(false), pointer(g) {}
  [[nodiscard]] bool isConst() const { return is_const; }
  shared_ptr<Geometry> ptr() { assert(!is_const); return pointer; }
  [[nodiscard]] shared_ptr<const Geometry> constptr() const {
    return is_const ? const_pointer : static_pointer_cast<const Geometry>(pointer);
  }
  shared_ptr<Geometry> asMutableGeometry() {
    if (isConst()) return shared_ptr<Geometry>(constptr() ? constptr()->copy() : nullptr);
    else return ptr();
  }
private:
  bool is_const;
  shared_ptr<Geometry> pointer;
  shared_ptr<const Geometry> const_pointer;
};

// This evaluates a node tree into concrete geometry usign an underlying geometry engine
// FIXME: Ideally, each engine should implement its own subtype. Instead we currently have
// multiple embedded engines with varoius methods of selecting the right one.
class StatelessGeometryEvaluator
{
public:
  StatelessGeometryEvaluator(const Tree& tree);

  ResultObject evaluate(const AbstractNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const AbstractIntersectionNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const AbstractPolyNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const LinearExtrudeNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const RotateExtrudeNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const RoofNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const ListNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const GroupNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const RootNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const LeafNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const TransformNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const CsgOpNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const CgalAdvNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const ProjectionNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const RenderNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const TextNode& node, const Geometry::Geometries &visitedchildren);
  ResultObject evaluate(const OffsetNode& node, const Geometry::Geometries &visitedchildren);

  [[nodiscard]] const Tree& getTree() const { return this->tree; }

private:

  bool isValidDim(const Geometry::GeometryItem& item, unsigned int& dim) const;
  std::vector<const Polygon2d *> collectChildren2D(const Geometry::Geometries &visitedchildren);
  Geometry::Geometries collectChildren3D(const Geometry::Geometries &visitedchildren);
  Polygon2d *applyMinkowski2D(const AbstractNode& node, const Geometry::Geometries &visitedchildren);
  Polygon2d *applyHull2D(const AbstractNode& node, const Geometry::Geometries &visitedchildren);
  Polygon2d *applyFill2D(const AbstractNode& node, const Geometry::Geometries &visitedchildren);
  Geometry *applyHull3D(const AbstractNode& node, const Geometry::Geometries &visitedchildren);
  void applyResize3D(CGAL_Nef_polyhedron& N, const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize, const Geometry::Geometries &visitedchildren);
  Polygon2d *applyToChildren2D(const AbstractNode& node, OpenSCADOperator op, const Geometry::Geometries &visitedchildren);
  ResultObject applyToChildren3D(const AbstractNode& node, OpenSCADOperator op, const Geometry::Geometries &visitedchildren);
  ResultObject applyToChildren(const AbstractNode& node, OpenSCADOperator op, const Geometry::Geometries &visitedchildren);
  shared_ptr<const Geometry> projectionCut(const ProjectionNode& node, const Geometry::Geometries &visitedchildren);
  shared_ptr<const Geometry> projectionNoCut(const ProjectionNode& node, const Geometry::Geometries &visitedchildren);

  const Tree& tree;
  shared_ptr<const Geometry> root;

public:
};

class GeometryEvaluator : public NodeVisitor
{
  template <class T>
  Response handle(State& state, const T& node, bool preferNef = false, int convexity = -1);

public:
  GeometryEvaluator(const Tree& tree);

  shared_ptr<const Geometry> evaluateGeometry(const AbstractNode& node, bool allownef);

  Response visit(State& state, const AbstractNode& node) override;
  Response visit(State& state, const AbstractIntersectionNode& node) override;
  Response visit(State& state, const AbstractPolyNode& node) override;
  Response visit(State& state, const LinearExtrudeNode& node) override;
  Response visit(State& state, const RotateExtrudeNode& node) override;
  Response visit(State& state, const RoofNode& node) override;
  Response visit(State& state, const ListNode& node) override;
  Response visit(State& state, const GroupNode& node) override;
  Response visit(State& state, const RootNode& node) override;
  Response visit(State& state, const LeafNode& node) override;
  Response visit(State& state, const TransformNode& node) override;
  Response visit(State& state, const CsgOpNode& node) override;
  Response visit(State& state, const CgalAdvNode& node) override;
  Response visit(State& state, const ProjectionNode& node) override;
  Response visit(State& state, const RenderNode& node) override;
  Response visit(State& state, const TextNode& node) override;
  Response visit(State& state, const OffsetNode& node) override;

  [[nodiscard]] const Tree& getTree() const { return this->tree; }

private:

  void smartCacheInsert(const AbstractNode& node, const shared_ptr<const Geometry>& geom);
  shared_ptr<const Geometry> smartCacheGet(const AbstractNode& node, bool preferNef);
  bool isSmartCached(const AbstractNode& node);

  void addToParent(const State& state, const AbstractNode& node, const shared_ptr<const Geometry>& geom);
  Response lazyEvaluateRootNode(State& state, const AbstractNode& node);

  std::map<int, Geometry::Geometries> visitedchildren;
  const Tree& tree;
  StatelessGeometryEvaluator geometryevaluator;
  shared_ptr<const Geometry> root;

public:
};
