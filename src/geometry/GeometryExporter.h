#pragma once

#include "node.h"
#include "assimp.h"
#include "GeometryEvaluator.h"
#include "manifoldutils.h"

class RenderGraph
{ 

  typedef shared_ptr<const Geometry> NodeRenderer(const Geometry::Geometries> &);

  struct RenderNode {
    shared_ptr<AbstractNode> node;
    NodeRef self;
    std::vector<NodeRef> deps;
    std::vector<NodeRef> rdeps;
    std::function<NodeRenderer> render;
    shared_ptr<const Geometry> result;

    RenderNode(NodeRef self, const shared_ptr<AbstractNode> &node, const std::vector<NodeRef> &deps, const std::function<NodeRenderer> &render) 
      : self(self), node(node), deps(deps), render(render) {}
  };

  std::vector<RenderNode> nodes;

public:
  // TODO: just use node.index()
  using NodeRef = size_t;

  NodeRef add_node(const shared_ptr<AbstractNode> &node, const std::vector<NodeRef> &deps, const std::function<NodeRenderer> &render) {
    auto ref = nodes.size();
    nodes.emplace_Back(ref, node, deps, render);
    for (auto d : deps) {
      nodes[d].rdeps.push_back(ref);
    }
    return ref;
  }

  std::vector<NodeRef> get_render_order() {
    std::stack<NodeRef> available_refs;
    std::vector<NodeRef> order;

    std::vector<size_t> incomplete_dep_counts(nodes.size());
    std::transform(nodes.begin(), nodes.end(), incomplete_dep_counts.begin(), [](const auto &n) { return n.dependencies.size(); });

    for (const auto &node = nodes) {
      if (!node.deps.empty()) {
        continue;
      }

      available_refs.push(node.ref);
    }
    while (!available_refs.empty()) {
      auto ref = available_refs.pop();
      order.push_back(ref);

      const auto &node = nodes[ref];
      for (auto rd : node.rdeps) {
        auto &incomplete_dep_count = incomplete_dep_counts[rd.ref];
        if ((--incomplete_dep_count) == 0) {
          available_refs.push(rd.ref);
        }
      }
    }

    assert((order.size() == nodes.size()) || !"Render graph isn't a tree!");
    return order;
  }

  void render() {
    // TODO(ochafik): Have a TBB branch.
    auto order = get_render_order();
    for (auto ref : order) {
      const auto &node = nodes[ref];

      Geometry::Geometries children;
      for (const auto dep_ref : node.deps) {
        auto &dep = nodes[dep_ref];
        children.emplace_back(dep.node, dep.result);
      }
      node.result = node.render(children);
    }
  }

  shared_ptr<const Geometry> get_render_result(NodeRef ref) {
    return nodes[ref].result;
  }
};

class GeometryExporter
{
  using NodeRef = RenderGraph::NodeRef;

  std::map<int, NodeRef> node_index_to_ref;
  // aiScene *scene = nullptr;
  RenderOptions options;
  const Tree& tree;
  GeometryEvaluator *geomevaluator;
  RenderGraph render_graph;
  // std::map<int, NodeRef> node_index_to_ref;
public:

  enum FormatCapabilities
  {
    None = 0,
    MultipleMeshes = 1,
    TransformNodes = 2,
    Materials = 4;
  };

  /*
    We use this class both to do "interactive" renderings on the Web (WASM),
    and to export to files (w/ multimaterial support for files that allow it).
  */
  struct RenderOptions {
    FormatCapabilities format_caps = 0;
    bool lazy_unions = false;
    bool is_preview = false;
  };

  struct RenderContext {
    std::optional<Transform4d> transform;
    std::optional<Color4f> color;
    bool is_percent = false; // NAME of % modifier?
    aiNode *node = nullptr;
  };

  GeometryExporter(const RenderOptions &options, const Tree &tree, GeometryEvaluator *geomevaluator)
    : options(options), tree(tree), geomevaluator(geomevaluator) {}

  aiScene *renderScene() {
    RenderContext context;
    aiScene *scene = new aiScene();
    
    return scene;
  }
  // aiScene *renderFrames(
  //   const std::vector<shared_ptr<const AbstractNode>> &frames,
  //   const RenderContext &context)
  // {

  // }

  std::vector<NodeRef> schedule(const std::vector<shared_ptr<AbstractNode>> &nodes,
    const RenderContext &context) {
    std::vector<NodeRef> res(nodes.size());
    std::transform(nodes.begin(), nodes.end(), res.begin(), [](auto &node) { return schedule(node, context); });
    return res;
  }

  NodeRef schedule(
    const shared_ptr<const AbstractNode> &node,
    const RenderContext &context)
  {
    // Response visit(State& state, const AbstractNode& node) override;
    // Response visit(State& state, const AbstractIntersectionNode& node) override;
    // Response visit(State& state, const AbstractPolyNode& node) override;
    // Response visit(State& state, const LinearExtrudeNode& node) override;
    // Response visit(State& state, const RotateExtrudeNode& node) override;
    // Response visit(State& state, const RoofNode& node) override;
    // Response visit(State& state, const ListNode& node) override;
    // Response visit(State& state, const GroupNode& node) override;
    // Response visit(State& state, const RootNode& node) override;
    // Response visit(State& state, const LeafNode& node) override;
    // Response visit(State& state, const TransformNode& node) override;
    // Response visit(State& state, const CsgOpNode& node) override;
    // Response visit(State& state, const CgalAdvNode& node) override;
    // Response visit(State& state, const ProjectionNode& node) override;
    // Response visit(State& state, const RenderNode& node) override;
    // Response visit(State& state, const TextNode& node) override;
    // Response visit(State& state, const OffsetNode& node) override;
    if (auto opNode = dynamic_pointer_cast<CsgOpNode>(node)) {
      return render_graph.add_node(schedule(opNode.getChildren()), [&](const auto &children) {
        if (op == OpenSCADOperator::UNION && options.lazy_unions) {
          context.node.
        }
        return op == OpenSCADOperator::MINKOWSKI
          ? ManifoldUtils::applyMinkowskiManifold(children)
          : ManifoldUtils::applyOperator3DManifold(children, op);
      });
    } else {
      return ender_graph.add_node(schedule({}, [&](const auto &children) {
        // Trick the geom evaluator into not reevaluating these children.
        for (const auto &child : children) {
          geomevaluator->smartCacheInsert(*child.first, child.second);
        }
        root_geom = geomevaluator.evaluateGeometry(*tree.root(), true);
      }
  }
};
