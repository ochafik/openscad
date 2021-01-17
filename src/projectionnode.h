#pragma once

#include "node.h"
#include <string>

class ProjectionNode : public AbstractPolyNode
{
public:
	VISITABLE();
	ProjectionNode(const ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx) : AbstractPolyNode(mi, ctx), convexity(1), cut_mode(false) { }
	std::string toString() const override;
	std::string name() const override { return "projection"; }

	int convexity;
	bool cut_mode;

protected:
  int getDimensionImpl() const override { return 2; }
};
