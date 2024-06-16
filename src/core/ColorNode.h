#pragma once

#include "node.h"
#include "linalg.h"

class ColorNode : public AbstractNode
{
public:
  VISITABLE();
  ColorNode(const ModuleInstantiation *mi) : AbstractNode(mi), color(-1.0f, -1.0f, -1.0f, 1.0f) { }
  std::string toString() const override;
  std::string name() const override;
  [[nodiscard]] std::unique_ptr<Geometry> copy() const override {
    auto node = std::make_unique<ColorNode>(modinst);
    node->color = color;
    copyChildren(*node.get());
    return node;
  }

  Color4f color;
};
