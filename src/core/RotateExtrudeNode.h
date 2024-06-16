#pragma once

#include "node.h"
#include "Value.h"

class RotateExtrudeNode : public AbstractPolyNode
{
public:
  VISITABLE();
  RotateExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
    convexity = 0;
    fn = fs = fa = 0;
    origin_x = origin_y = scale = 0;
    angle = 360;
  }
  std::string toString() const override;
  std::string name() const override { return "rotate_extrude"; }
  [[nodiscard]] std::unique_ptr<AbstractNode> copy() const override {
    auto node = std::make_unique<RotateExtrudeNode>(modinst);
    node->convexity = convexity;
    node->fn = fn;
    node->fs = fs;
    node->fa = fa;
    node->origin_x = origin_x;
    node->origin_y = origin_y;
    node->scale = scale;
    node->angle = angle;
    node->filename = filename;
    node->layername = layername;
    copyChildren(*node.get());
    return node;
  }

  int convexity;
  double fn, fs, fa;
  double origin_x, origin_y, scale, angle;
  Filename filename;
  std::string layername;
};
