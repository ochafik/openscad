#include "localscope.h"
#include "modcontext.h"
#include "module.h"
#include "csgops.h"
#include "ModuleInstantiation.h"
#include "node.h"
#include "UserModule.h"
#include "expression.h"
#include "function.h"
#include "annotation.h"
#include "UserModule.h"
#include "transformnode.h"

// TODO(ochafik): Come up with a safer way to transform node trees.
template <class T>
void flatten_and_delete(T *list, std::vector<AbstractNode *> &out)
{
	if (list->modinst && list->modinst->hasSpecialTags()) {
		out.push_back(list);
		return;
	}
	for (auto child : list->children) {
		if (child) {
			if (auto sublist = dynamic_cast<T *>(child)) {
				flatten_and_delete(sublist, out);
			}
			else {
				out.push_back(child);
			}
		}
	}
	list->children.clear();
	delete list;
}

AbstractNode *simplify_tree(AbstractNode *root)
{
  for (auto it = root->children.begin(); it != root->children.end(); it++) {
    auto child = *it;
    auto simpler_child = simplify_tree(child);
    *it = simpler_child;
  }

	if (root->modinst && root->modinst->hasSpecialTags()) {
    // Opt out of this mess.
		return root;
	}

  auto list = dynamic_cast<ListNode *>(root);
  auto group = dynamic_cast<GroupNode *>(root);

  auto mi = root->modinst;

  // if ((list || group) && root->children.size() == 1) {
  //   // List or group w/ a single child? Return that child.
  //   auto child = root->children[0];
  //   root->children.clear();
  //   delete root;
  //   return child;
  // } else
  if (list) {
    // Flatten lists.
    auto node = new ListNode(mi, shared_ptr<EvalContext>());
    flatten_and_delete(list, node->children);
    // if (node->children.size() == 1) {
    //   auto child = node->children[0];
    //   node->children.clear();
    //   delete node;
    //   return child;
    // }
    return node;
  } else if (group) {
    // Flatten groups.
    auto node = new GroupNode(mi, shared_ptr<EvalContext>());
    flatten_and_delete(group, node->children);
    // if (node->children.size() == 1) {
    //   auto child = node->children[0];
    //   node->children.clear();
    //   delete node;
    //   return child;
    // }
    return node;
  } else if (auto transform = dynamic_cast<TransformNode *>(root)) {
    // Push transforms down.
    auto has_any_specially_tagged_child = false;
    auto has_any_transform_child = false;
    for (auto child : transform->children) {
      if (dynamic_cast<TransformNode *>(child)) has_any_transform_child = true;
      if (child->modinst && child->modinst->hasSpecialTags()) has_any_specially_tagged_child = true;
    }

    if (!has_any_specially_tagged_child && (transform->children.size() > 1 || has_any_transform_child)) {
      std::vector<AbstractNode *> children;
      for (auto child : transform->children) {
        if (auto child_transform = dynamic_cast<TransformNode *>(child)) {
          child_transform->matrix = transform->matrix * child_transform->matrix;
          children.push_back(child_transform);
        } else {
          auto clone = new TransformNode(mi, shared_ptr<EvalContext>(), transform->verbose_name());
          clone->matrix = transform->matrix;
          clone->children.push_back(child);
          children.push_back(clone);
        }
      }

      transform->children.clear();
      delete transform;

      if (children.size() == 1) {
        return children[0];
      }
      AbstractNode *new_parent;
      if (Feature::ExperimentalLazyUnion.is_enabled())
        new_parent = new ListNode(mi, shared_ptr<EvalContext>());
      else
        new_parent = new GroupNode(mi, shared_ptr<EvalContext>());

      new_parent->children = children;

      return new_parent;
    }
  } else if (auto op_node = dynamic_cast<CsgOpNode*>(node)) {
    if (Feature::ExperimentalDifferenceUnion.is_enabled() &&
        op_node->type == OpenSCADOperator::DIFFERENCE && original_child_count > 2) {
#ifdef DEBUG
      std::cerr << "[simplify_tree] Grouping " << (original_child_count - 1) << " subtracted terms of a difference into a union\n";
#endif
      auto union_node = new CsgOpNode(mi, std::shared_ptr<EvalContext>(), OpenSCADOperator::UNION);
      for (size_t i = 1; i < original_child_count; i++) {
        union_node->children.push_back(node->children[i]);
      }
      node->children.resize(1);
      node->children.push_back(union_node);
    }
  }

  return root;
}
