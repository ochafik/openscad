#pragma once

#include "node.h"
#include <string>

class RenderNode : public AbstractNode
{
public:
  VISITABLE();
  RenderNode(const ModuleInstantiation *mi) : AbstractNode(mi) { }
  std::string toString() const override;
  std::string name() const override { return "render"; }
  [[nodiscard]] std::unique_ptr<AbstractNode> copy() const override {
    auto node = std::make_unique<RenderNode>(modinst);
    node->convexity = convexity;
    copyChildren(*node.get());
    return node;
  }

  int convexity{1};
};
