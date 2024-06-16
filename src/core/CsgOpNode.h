#pragma once

#include "node.h"
#include "enums.h"

class CsgOpNode : public AbstractNode
{
public:
  VISITABLE();
  OpenSCADOperator type;
  CsgOpNode(const ModuleInstantiation *mi, OpenSCADOperator type) : AbstractNode(mi), type(type) { }
  std::string toString() const override;
  std::string name() const override;
  [[nodiscard]] std::unique_ptr<Geometry> copy() const override {
    auto node = std::make_unique<CsgOpNode>(modinst, type);
    node->type = type;
    copyChildren(*node.get());
    return node;
  }
};
