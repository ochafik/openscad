#pragma once

#include "node.h"
#include "linalg.h"

enum class CgalAdvType {
  MINKOWSKI,
  HULL,
  FILL,
  RESIZE
};

class CgalAdvNode : public AbstractNode
{
public:
  VISITABLE();
  CgalAdvNode(const ModuleInstantiation *mi, CgalAdvType type) : AbstractNode(mi), type(type) {
  }
  std::string toString() const override;
  std::string name() const override;
  [[nodiscard]] std::unique_ptr<Geometry> copy() const override {
    auto node = std::make_unique<CgalAdvNode>(modinst, type);
    node->convexity = convexity;
    node->newsize = newsize;
    node->autosize = autosize;
    node->type = type;
    copyChildren(*node.get());
    return node;
  }

  unsigned int convexity{1};
  Vector3d newsize;
  Eigen::Matrix<bool, 3, 1> autosize;
  CgalAdvType type;
};
