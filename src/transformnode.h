#pragma once

#include "node.h"
#include "linalg.h"

class TransformNode : public AbstractListLikeNode
{
public:
	VISITABLE();
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	TransformNode(const ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx, const std::string &verbose_name);
	std::string toString() const override;
	std::string name() const override;
	std::string verbose_name() const override;
	Transform3d matrix;


  // TODO(ochafik): Move up to AbstractNode once we support cloning all node types!
  virtual AbstractNode *normalize() override {
    if (Feature::ExperimentalPushTransforms.is_enabled()) {

      const auto normalized_children = normalizedChildren();

      auto has_tagged_child = false;
      auto has_transform_child = false;
      for (auto child : normalized_children) {
        if (dynamic_cast<TransformNode *>(child)) has_transform_child = true;
        if (child->modinst && child->modinst->hasSpecialTags()) has_tagged_child = true;
      }

      if (!has_tagged_child && (normalized_children.size() > 1 || has_transform_child)) {
        std::vector<AbstractNode *> transformed_children;
        for (auto child : normalized_children) {
          if (auto child_transform = dynamic_cast<TransformNode *>(child)) {
            auto new_transform = cloneWithoutChildren();
            new
            child_transform->matrix = transform->matrix * child_transform->matrix;
            children.push_back(child_transform);
          } else {
            auto clone = new TransformNode(mi, shared_ptr<EvalContext>(), transform->verbose_name());
            clone->matrix = transform->matrix;
            clone->children.push_back(child);
            children.push_back(clone);
          }
        }
        normalized_children = transformed_children;
      }

      auto needs_cloning = normalized_children != children;


        transform->children.clear();
        delete transform;

        // LOG(message_group::None, Location::NONE, "",
        //   "[transform_tree] Pushing TransformNode down onto %1$d children (of which %2$d were TransformNodes)", children.size(), transform_children_count);

        if (children.size() == 1) {
          // We've already pushed any transform down, so it's safe to bubble our only child up.
          return children[0];
        }
        AbstractNode *new_parent;
        if (Feature::ExperimentalLazyUnion.is_enabled()) new_parent = new ListNode(mi, shared_ptr<EvalContext>());
        else new_parent = new GroupNode(mi, shared_ptr<EvalContext>());

        new_parent->children = children;

        return new_parent;
      }

      if (needs_cloning) {
        AbstractNode* clone = cloneWithoutChildren();
        clone->children = normalized_children;
        return clone;
      }

      if (auto transform = dynamic_cast<TransformNode *>(node)) {
        // Push transforms down.
        auto has_tagged_child = false;
        auto transform_children_count = false;
        for (auto child : transform->children) {
          if (dynamic_cast<TransformNode *>(child)) transform_children_count = true;
          if (child->modinst && child->modinst->hasSpecialTags()) has_tagged_child = true;
        }

      }
    }
    return AbstractListLikeNode::normalize();
  }

protected:
  // TODO(ochafik): Move this to AbstractNode once we support all nodes.
  const std::shared_ptr<EvalContext> ctx;

  virtual AbstractNode* cloneWithoutChildren() const = 0;
  virtual bool canFlattenChild(const AbstractNode*child) const = 0;

  std::vector<AbstractNode*> normalizeChildren() override {
    std::vector<AbstractNode*> normalized_children;
    for (auto child : AbstractNode::normalizeChildren()) {
      if (canFlattenChild(child)) {
        for (auto grandchild : child->children) {
          normalized_children.push_back(grandchild);
        }
      } else {
        normalized_children.push_back(child);
      }
    }
    return normalized_children;
  }

protected:
  AbstractNode* cloneWithoutChildren() const override {
    auto node = new TransformNode(modinst, ctx);
    node->matrix = matrix;
    return node;
  }

  bool canFlattenChild(const AbstractNode*child) const override {
    return dynamic_cast<const ListNode*>(child);
  }


private:
	const std::string _name;
};
