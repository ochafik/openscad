#pragma once

#include "node.h"
#include <string>

class ProjectionNode : public AbstractPolyNode
{
public:
  VISITABLE();
  ProjectionNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) { }
  std::string toString() const override;
  std::string name() const override { return "projection"; }
  [[nodiscard]] std::unique_ptr<AbstractNode> copy() const override {
    auto node = std::make_unique<ProjectionNode>(modinst);
    node->convexity = convexity;
    node->cut_mode = cut_mode;
    copyChildren(*node.get());
    return node;
  }

  int convexity{1};
  bool cut_mode{false};
};
