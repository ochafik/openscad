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

class StatelessGeometryEvaluator
{
public:
  StatelessGeometryEvaluator(const Tree& tree);

  ResultObject evaluateGeometry(const AbstractNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const AbstractIntersectionNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const AbstractPolyNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const LinearExtrudeNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const RotateExtrudeNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const RoofNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const ListNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const GroupNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const RootNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const LeafNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const TransformNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const CsgOpNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const CgalAdvNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const ProjectionNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const RenderNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const TextNode& node, const Geometry::Geometries &children);
  ResultObject evaluateGeometry(const OffsetNode& node, const Geometry::Geometries &children);

  [[nodiscard]] const Tree& getTree() const { return this->tree; }

private:
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

  
  void smartCacheInsert(const AbstractNode& node, const shared_ptr<const Geometry>& geom);
  shared_ptr<const Geometry> smartCacheGet(const AbstractNode& node, bool preferNef);
  bool isSmartCached(const AbstractNode& node);
  bool isValidDim(const Geometry::GeometryItem& item, unsigned int& dim) const;
  std::vector<const Polygon2d *> collectChildren2D(const AbstractNode& node, const Geometry::Geometries &children);
  Geometry::Geometries collectChildren3D(const AbstractNode& node, const Geometry::Geometries &children);
  Polygon2d *applyMinkowski2D(const AbstractNode& node);
  Polygon2d *applyHull2D(const AbstractNode& node);
  Polygon2d *applyFill2D(const AbstractNode& node);
  Geometry *applyHull3D(const AbstractNode& node);
  void applyResize3D(CGAL_Nef_polyhedron& N, const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize);
  Polygon2d *applyToChildren2D(const AbstractNode& node, OpenSCADOperator op);
  ResultObject applyToChildren3D(const AbstractNode& node, OpenSCADOperator op);
  ResultObject applyToChildren(const AbstractNode& node, OpenSCADOperator op);
  shared_ptr<const Geometry> projectionCut(const ProjectionNode& node);
  shared_ptr<const Geometry> projectionNoCut(const ProjectionNode& node);

  void addToParent(const State& state, const AbstractNode& node, const shared_ptr<const Geometry>& geom);
  Response lazyEvaluateRootNode(State& state, const AbstractNode& node);

  const Tree& tree;
  shared_ptr<const Geometry> root;

public:
};


class GeometryEvaluator
{
public:
  GeometryEvaluator(const Tree& tree);

  shared_ptr<const Geometry> evaluateGeometry(const AbstractNode& node, bool allownef);

  template <class T>
  Response handle(State& state, const T& node, bool preferNef = false, int convexity = -1)
  {
    if (state.isPrefix()) {
      if (isSmartCached(node)) return Response::PruneTraversal;
      if (preferNef) state.setPreferNef(true); // Improve quality of CSG by avoiding conversion loss
    }
    if (state.isPostfix()) {
      shared_ptr<const Geometry> geom;
      if (!isSmartCached(node)) {
        auto res = statelessevaluator.evaluateGeometry(node, this->visitedchildren[node.index()]);
        if (convexity >= 0) {
          auto mutableGeom = res.asMutableGeometry();
          if (mutableGeom) mutableGeom->setConvexity(convexity);
          geom = mutableGeom;
        } else {
          geom = res;
        }
      } else {
        geom = smartCacheGet(node, state.preferNef());
      }
      node.progress_report();
      addToParent(state, node, geom);
    }
    return Response::ContinueTraversal;
  }

  Response visit(State& state, const AbstractNode& node) override { return handle(state, node); }
  Response visit(State& state, const AbstractIntersectionNode& node) override { return handle(state, node); }
  Response visit(State& state, const AbstractPolyNode& node) override { return handle(state, node); }
  Response visit(State& state, const LinearExtrudeNode& node) override { return handle(state, node); }
  Response visit(State& state, const RotateExtrudeNode& node) override { return handle(state, node); }
  Response visit(State& state, const RoofNode& node) override { return handle(state, node); }
  Response visit(State& state, const ListNode& node) override { return handle(state, node); }
  Response visit(State& state, const GroupNode& node) override { return handle(state, node); }
  Response visit(State& state, const RootNode& node) override { return handle(state, node); }
  Response visit(State& state, const LeafNode& node) override { return handle(state, node); }
  Response visit(State& state, const TransformNode& node) override { return handle(state, node); }
  Response visit(State& state, const CsgOpNode& node) override { return handle(state, node); }
  Response visit(State& state, const CgalAdvNode& node) override { return handle(state, node); }
  Response visit(State& state, const ProjectionNode& node) override { return handle(state, node); }
  Response visit(State& state, const RenderNode& node) override { return handle(state, node, /* preferNef= */ true, node.convexity); }
  Response visit(State& state, const TextNode& node) override { return handle(state, node); }
  Response visit(State& state, const OffsetNode& node) override { return handle(state, node); }

  [[nodiscard]] const Tree& getTree() const { return this->tree; }

private:

  void smartCacheInsert(const AbstractNode& node, const shared_ptr<const Geometry>& geom);
  shared_ptr<const Geometry> smartCacheGet(const AbstractNode& node, bool preferNef);
  bool isSmartCached(const AbstractNode& node);

  void addToParent(const State& state, const AbstractNode& node, const shared_ptr<const Geometry>& geom);
  Response lazyEvaluateRootNode(State& state, const AbstractNode& node);

  std::map<int, Geometry::Geometries> visitedchildren;
  const Tree& tree;
  StatelessGeometryEvaluator statelessevaluator;
  shared_ptr<const Geometry> root;

public:
};
