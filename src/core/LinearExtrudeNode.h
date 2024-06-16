#pragma once

#include "node.h"
#include "Value.h"
#include "linalg.h"

class LinearExtrudeNode : public AbstractPolyNode
{
public:
  VISITABLE();
  LinearExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
  }
  std::string toString() const override;
  std::string name() const override { return "linear_extrude"; }
  [[nodiscard]] std::unique_ptr<Geometry> copy() const override {
    auto node = std::make_unique<LinearExtrudeNode>(modinst, _name);
    node->height = height;
    node->origin_x = origin_x;
    node->origin_y = origin_y;
    node->fn = fn;
    node->fs = fs;
    node->fa = fa;
    node->scale_x = scale_x;
    node->scale_y = scale_y;
    node->twist = twist;
    node->convexity = convexity;
    node->slices = slices;
    node->segments = segments;
    node->has_twist = has_twist;
    node->has_slices = has_slices;
    node->has_segments = has_segments;
    node->center = center;
    node->filename = filename;
    node->layername = layername;
    copyChildren(*node.get());
    return node;
  }

  Vector3d height=Vector3d(0, 0, 100);
  double origin_x = 0.0, origin_y = 0.0;
  double fn = 0.0, fs = 0.0, fa = 0.0;
  double scale_x = 1.0, scale_y = 1.0;
  double twist = 0.0;
  unsigned int convexity = 1u;
  unsigned int slices = 1u, segments = 0u;
  bool has_twist = false, has_slices = false, has_segments = false;
  bool center = false;

  Filename filename;
  std::string layername;
};
