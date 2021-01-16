#pragma once

#include "node.h"
#include "linalg.h"

class ColorNode : public AbstractNode
{
public:
	VISITABLE();
	ColorNode(const ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx) : AbstractNode(mi, ctx), color(-1.0f, -1.0f, -1.0f, 1.0f) { }
	std::string toString() const override;
	std::string name() const override;
	AbstractNode *clone_template(const ModuleInstantiation *mi,
															 const std::shared_ptr<EvalContext> &ctx) const override
	{
		assert(children.size() == 0);
		auto clone = new ColorNode(mi, ctx);
		clone->color = color;
		return clone;
	}

	Color4f color;
};
