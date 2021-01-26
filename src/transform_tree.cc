#include "csgops.h"
#include "feature.h"
#include "ModuleInstantiation.h"
#include "transformnode.h"
#include "printutils.h"

/*! Either flattens the list (or group, or whatever commutative thing it may),
 *  or outputs that list (if, say, it has special tags).
 */
template <class T>
void flatten_and_delete(T *list, std::vector<AbstractNode *> &out)
{
	if (list->modinst && list->modinst->hasSpecialTags()) {
		out.push_back(list);
		return;
	}
	for (auto child : list->children) {
		if (child) {
      // Do we have a sublist of the same type T as list?
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

/*! Destructively transforms the tree according to various enabled features.
 *  Generates nodes that may lack proper debug info in the process, so this
 *  definitely should be considererd highly experimental.
 */
AbstractNode *transform_tree(AbstractNode *node)
{
  auto original_child_count = node->children.size();

  auto has_child_with_special_tags = false;
  for (auto it = node->children.begin(); it != node->children.end(); it++) {
    auto child = *it;
    auto simpler_child = transform_tree(child);
    if (simpler_child && simpler_child->modinst && simpler_child->modinst->hasSpecialTags()) {
      has_child_with_special_tags = true;
    }
    *it = simpler_child;
  }

	if (node->modinst && node->modinst->hasSpecialTags()) {
    LOG(message_group::None, Location::NONE, "", "[transform_tree] Ignoring tree with special tags");
		return node;
	}

  auto mi = node->modinst;

  if (Feature::ExperimentalFlattenChildren.is_enabled()) {
    if (auto list = dynamic_cast<ListNode *>(node)) {
      // Flatten lists.
      auto new_node = new ListNode(mi, shared_ptr<EvalContext>());
      flatten_and_delete(list, new_node->children);
      if (original_child_count != new_node->children.size()) {
        LOG(message_group::None, Location::NONE, "",
          "[transform_tree] Flattened ListNode (%1$d -> %2$d children)", original_child_count, new_node->children.size());
      }
      if (new_node->children.size() == 1) {
        LOG(message_group::None, Location::NONE, "", "[transform_tree] Dropping single-child ListNode\n");
        auto child = new_node->children[0];
        new_node->children.clear();
        delete new_node;
        return child;
      }
      return new_node;
    } else if (auto group = dynamic_cast<GroupNode *>(node)) {
      if (!dynamic_cast<RootNode *>(node)) {
        // Flatten groups.
        // TODO(ochafik): Flatten root as a... Group unless we're in lazy-union mode (then as List)
        auto new_node = new GroupNode(mi, shared_ptr<EvalContext>());
        flatten_and_delete(group, new_node->children);
        if (original_child_count != new_node->children.size()) {
          LOG(message_group::None, Location::NONE, "",
            "[transform_tree] Flattened GroupNode (%1$d -> %2$d children)", original_child_count, new_node->children.size());
        }
        if (new_node->children.size() == 1) {
          LOG(message_group::None, Location::NONE, "", "[transform_tree] Dropping single-child GroupNode\n");
          auto child = new_node->children[0];
          new_node->children.clear();
          delete new_node;
          return child;
        }
        return new_node;
      }
    } else if (auto csg = dynamic_cast<CsgOpNode *>(node)) {
      auto new_node = new CsgOpNode(mi, shared_ptr<EvalContext>(), csg->type);
      list = new ListNode(mi, shared_ptr<EvalContext>());
      list->children = csg->children;
      csg->children.clear();
      flatten_and_delete(list, new_node->children);
      if (original_child_count != new_node->children.size()) {
        LOG(message_group::None, Location::NONE, "",
          "[transform_tree] Flattened CsgOpNode (%1$d -> %2$d children)", original_child_count, new_node->children.size());
      }

      if (new_node->children.size() == 1) {
        LOG(message_group::None, Location::NONE, "", "[transform_tree] Dropping single-child CsgOpNode\n");
        auto child = new_node->children[0];
        new_node->children.clear();
        delete new_node;
        return child;
      }

      return new_node;
    }
  }

  if (Feature::ExperimentalPushTransforms.is_enabled()) {
    if (auto transform = dynamic_cast<TransformNode *>(node)) {
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

        LOG(message_group::None, Location::NONE, "",
          "[transform_tree] Pushing TransformNode down onto %1$d children (of which %2$d were TransformNodes)", children.size(), transform_children_count);

        if (children.size() == 1) {
          return children[0];
        }
        AbstractNode *new_parent;
        if (Feature::ExperimentalLazyUnion.is_enabled()) new_parent = new ListNode(mi, shared_ptr<EvalContext>());
        else new_parent = new GroupNode(mi, shared_ptr<EvalContext>());

        new_parent->children = children;

        return new_parent;
      }
    }
  }

  // No changes (*sighs*)
  return node;
}