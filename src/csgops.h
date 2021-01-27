#pragma once

#include "node.h"
#include "enums.h"

class CsgOpNode : public AbstractListLikeNode
{
public:
	VISITABLE();
	OpenSCADOperator type;
	CsgOpNode(const ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx, OpenSCADOperator type) : AbstractListLikeNode(mi, ctx), type(type) { }
	std::string toString() const override;
	std::string name() const override;

protected:
  AbstractNode* cloneWithoutChildren() const override {
    return new CsgOpNode(modinst, ctx);
  }
  bool canFlattenChild(const AbstractNode*child) const override {
    if (type == OpenSCADOperator::UNION || type == OpenSCADOperator::INTERSECTION || type == OpenSCADOperator::DIFFERENCE) {
      if (auto csg = dynamic_cast<const CsgOpNode*>(child)) {
        return csg->type == type;
      }
    }
    return false;
  }
};
