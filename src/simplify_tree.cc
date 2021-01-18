#include "csgops.h"
#include "feature.h"
#include "ModuleInstantiation.h"
#include "transformnode.h"
#include "printutils.h"

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

AbstractNode *simplify_tree(AbstractNode *node)
{
  auto original_child_count = node->children.size();

  for (auto it = node->children.begin(); it != node->children.end(); it++) {
    auto child = *it;
    auto simpler_child = simplify_tree(child);
    *it = simpler_child;
  }

	if (node->modinst && node->modinst->hasSpecialTags()) {
    // Opt out of this mess.
#ifdef DEBUG
    LOG(message_group::None, Location::NONE, "",
      "[simplify_tree] Ignoring tree with special tags");
#endif
		return node;
	}

  auto list = dynamic_cast<ListNode *>(node);
  auto group = dynamic_cast<GroupNode *>(node);
  auto root = dynamic_cast<RootNode *>(node);
  auto csg = dynamic_cast<CsgOpNode *>(node);

  auto mi = node->modinst;

  if (list) {
    // Flatten lists.
    auto new_node = new ListNode(mi, shared_ptr<EvalContext>());
    flatten_and_delete(list, new_node->children);
#ifdef DEBUG
    if (original_child_count != new_node->children.size()) {
      LOG(message_group::None, Location::NONE, "",
        "[simplify_tree] Flattened ListNode (%1$d -> %2$d children)", original_child_count, new_node->children.size());
    }
#endif
    if (new_node->children.size() == 1) {
#ifdef DEBUG
      LOG(message_group::None, Location::NONE, "", "[simplify_tree] Dropping single-child ListNode\n");
#endif
      auto child = new_node->children[0];
      new_node->children.clear();
      delete new_node;
      return child;
    }
    return new_node;
  } else if (group && !root) {
    // Flatten groups.
    // TODO(ochafik): Flatten root as a... Group unless we're in lazy-union mode (then as List)
    auto new_node = new GroupNode(mi, shared_ptr<EvalContext>());
    flatten_and_delete(group, new_node->children);
#ifdef DEBUG
    if (original_child_count != new_node->children.size()) {
      LOG(message_group::None, Location::NONE, "",
        "[simplify_tree] Flattened GroupNode (%1$d -> %2$d children)", original_child_count, new_node->children.size());
    }
#endif
    if (new_node->children.size() == 1) {
#ifdef DEBUG
      LOG(message_group::None, Location::NONE, "", "[simplify_tree] Dropping single-child GroupNode\n");
#endif
      auto child = new_node->children[0];
      new_node->children.clear();
      delete new_node;
      return child;
    }
    return new_node;
  } else if (auto transform = dynamic_cast<TransformNode *>(node)) {
    // Push transforms down.
    auto has_any_specially_tagged_child = false;
    auto transform_children_count = false;
    for (auto child : transform->children) {
      if (dynamic_cast<TransformNode *>(child)) transform_children_count = true;
      if (child->modinst && child->modinst->hasSpecialTags()) has_any_specially_tagged_child = true;
    }

    if (!has_any_specially_tagged_child && (transform->children.size() > 1 || transform_children_count)) {
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

#ifdef DEBUG
      LOG(message_group::None, Location::NONE, "",
        "[simplify_tree] Pushing TransformNode down onto %1$d children (of which %2$d were TransformNodes)", children.size(), transform_children_count);
#endif

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
  } else if (csg) {
    // Flatten children.
    auto new_node = new CsgOpNode(mi, shared_ptr<EvalContext>(), csg->type);
    list = new ListNode(mi, shared_ptr<EvalContext>());
    list->children = csg->children;
    csg->children.clear();
    flatten_and_delete(list, new_node->children);
#ifdef DEBUG
    if (original_child_count != new_node->children.size()) {
      LOG(message_group::None, Location::NONE, "",
        "[simplify_tree] Flattened CsgOpNode (%1$d -> %2$d children)", original_child_count, new_node->children.size());
    }
#endif

    if (new_node->children.size() == 1) {
#ifdef DEBUG
      LOG(message_group::None, Location::NONE, "", "[simplify_tree] Dropping single-child CsgOpNode\n");
#endif
      auto child = new_node->children[0];
      new_node->children.clear();
      delete new_node;
      return child;
    }
#ifdef DEBUG
      LOG(message_group::None, Location::NONE, "",
        "[simplify_tree] Found csg node with %1$d children", new_node->children.size());
#endif
    if (Feature::ExperimentalDifferenceUnion.is_enabled() &&
        new_node->type == OpenSCADOperator::DIFFERENCE && new_node->children.size() > 2) {
#ifdef DEBUG
      LOG(message_group::None, Location::NONE, "",
        "[simplify_tree] Grouping %1$d subtracted terms of a difference into a union", new_node->children.size() - 1);
#endif
      auto union_node = new CsgOpNode(mi, std::shared_ptr<EvalContext>(), OpenSCADOperator::UNION);
      for (size_t i = 1; i < new_node->children.size(); i++) {
        union_node->children.push_back(new_node->children[i]);
      }
      new_node->children.resize(1);
      new_node->children.push_back(union_node);
    }
    return new_node;
  }

  // No changes (*sighs*)
  return node;
}
