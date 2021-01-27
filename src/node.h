#pragma once

#include <vector>
#include <string>
#include <deque>
#include "BaseVisitable.h"
#include "AST.h"

extern int progress_report_count;
extern void (*progress_report_f)(const class AbstractNode*, void*, int);
extern void *progress_report_vp;

void progress_report_prep(class AbstractNode *root, void (*f)(const class AbstractNode *node, void *vp, int mark), void *vp);
void progress_report_fin();

class EvalContext;

/*!

	The node tree is the result of evaluation of a module instantiation
	tree.  Both the module tree and the node tree are regenerated from
	scratch for each compile.

 */
class AbstractNode : public BaseVisitable
{
	// FIXME: the idx_counter/idx is mostly (only?) for debugging.
	// We can hash on pointer value or smth. else.
  //  -> remove and
	// use smth. else to display node identifier in CSG tree output?
	static size_t idx_counter;   // Node instantiation index
public:
	VISITABLE();
	AbstractNode(const class ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx);
	~AbstractNode();
	virtual std::string toString() const;
	/*! The 'OpenSCAD name' of this node, defaults to classname, but can be
	    overloaded to provide specialization for e.g. CSG nodes, primitive nodes etc.
	    Used for human-readable output. */
	virtual std::string name() const = 0;
  /*| When a more specific name for user interaction shall be used, such as module names,
      the verbose name shall be overloaded. */
  virtual std::string verbose_name() const { return this->name(); }
  /*! Should return a Geometry instance describing the node. Returns nullptr if smth.
		goes wrong. This is only called by PolySetEvaluator, to make sure polysets
		are inserted into the cache*/
	virtual class Geometry *evaluate_geometry(class PolySetEvaluator *) const { return nullptr; }

	const std::vector<AbstractNode*> &getChildren() const {
		return this->children;
	}
	size_t index() const { return this->idx; }

	static void resetIndexCounter() { idx_counter = 1; }

	// FIXME: Make protected
	std::vector<AbstractNode*> children;
	const ModuleInstantiation *modinst;

	// progress_mark is a running number used for progress indication
	// FIXME: Make all progress handling external, put it in the traverser class?
	int progress_mark;
	void progress_prepare();
	void progress_report() const;

	int idx; // Node index (unique per tree)

	const Location location;

	const AbstractNode *getNodeByID(int idx, std::deque<const AbstractNode *> &path) const;

  /*!
   * Does a bunch of stuff, depending on features enabled:
   * - Flatten nested lists, groups, CSG ops, transforms
   * - Potentially in the future, may swap operations around to optimize for
   *   rendering speed / parallelizability.
   *
   * This might return itself if there's not change.
   *
   * TODO(ochafik): Can't make this method const b/c children doesn't contain
   * const pointers. Fix this?
   */
  virtual AbstractNode *normalize() {
    return this;
  }

protected:
  /*
   * TODO(ochafik): Can't make this method const b/c children doesn't contain
   * const pointers. Fix this?
   */
  virtual std::vector<AbstractNode*> normalizedChildren() {
    std::vector<AbstractNode*> normalized_children;
    for (auto child : children) {
      normalized_children.push_back(child ? child->normalize() : child);
    }
    return normalized_children;
  }
};

class AbstractIntersectionNode : public AbstractNode
{
public:
	VISITABLE();
	AbstractIntersectionNode(const ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx) : AbstractNode(mi, ctx) { };
	~AbstractIntersectionNode() { };
	std::string toString() const override;
	std::string name() const override;
};

class AbstractPolyNode : public AbstractNode
{
public:
	VISITABLE();
	AbstractPolyNode(const ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx) : AbstractNode(mi, ctx) { };
	~AbstractPolyNode() { };

	enum class render_mode_e {
		RENDER_CGAL,
		RENDER_OPENCSG
	};
};

class AbstractListLikeNode : public AbstractNode {
public:
	AbstractListLikeNode(const class ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx) : AbstractNode(mi, ctx), ctx(ctx) {}
	~AbstractListLikeNode() {}

  // TODO(ochafik): Move up to AbstractNode once we support cloning all node types!
  virtual AbstractNode *normalize() override {
    const auto normalized_children = normalizedChildren();
    // Yup, just test two vectors of pointers for equality.
    if (normalized_children != children) {
      AbstractNode* clone = cloneWithoutChildren();
      clone->children = normalized_children;
      return clone;
    }
    return this;
  }

protected:
  // TODO(ochafik): Move this to AbstractNode once we support all nodes.
  const std::shared_ptr<EvalContext> ctx;

  virtual AbstractNode* cloneWithoutChildren() const = 0;
  virtual bool canFlattenChild(const AbstractNode*child) const = 0;

  std::vector<AbstractNode*> normalizedChildren() override {
    std::vector<AbstractNode*> normalized_children;
    for (auto child : AbstractNode::normalizedChildren()) {
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
};

/*!
	Used for organizing objects into lists which should not be grouped but merely
	unpacked by the parent node.
 */
class ListNode : public AbstractListLikeNode
{
public:
	VISITABLE();
	ListNode(const class ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx, const std::string &name="") : AbstractListLikeNode(mi, ctx), _name(name) { }
	~ListNode() { }
	std::string name() const override;
  std::string verbose_name() const override;

protected:
  AbstractNode* cloneWithoutChildren() const override {
    return new ListNode(modinst, ctx);
  }
  bool canFlattenChild(const AbstractNode*child) const override {
    return dynamic_cast<const ListNode*>(child);
  }

private:
	const std::string _name;
};

/*!
  Logically groups objects together. Used as a way of passing
	objects around without having to perform unions on them.
 */
class GroupNode : public AbstractListLikeNode
{
public:
	VISITABLE();
	GroupNode(const class ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx, const std::string &name="") : AbstractListLikeNode(mi, ctx), _name(name) { }
	~GroupNode() { }
	std::string name() const override;
  std::string verbose_name() const override;

protected:
  AbstractNode* cloneWithoutChildren() const override {
    return new GroupNode(modinst, ctx);
  }
  bool canFlattenChild(const AbstractNode*child) const override {
    return dynamic_cast<const GroupNode*>(child);
  }
private:
	const std::string _name;
};

/*!
	Only instantiated once, for the top-level file.
*/
class RootNode : public GroupNode
{
public:
	VISITABLE();

	RootNode(const class ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx) : GroupNode(mi, ctx) { }
	~RootNode() { }
	std::string name() const override;

protected:
  AbstractNode* cloneWithoutChildren() const override {
    return new RootNode(modinst, ctx);
  }
  bool canFlattenChild(const AbstractNode*child) const override {
    return false; // TODO(ochafik): check this
    // return dynamic_cast<const GroupNode*>(child);
  }
};

class LeafNode : public AbstractPolyNode
{
public:
	VISITABLE();
	LeafNode(const ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx) : AbstractPolyNode(mi, ctx) { };
	~LeafNode() { };
	virtual const class Geometry *createGeometry() const = 0;
};

std::ostream &operator<<(std::ostream &stream, const AbstractNode &node);
AbstractNode *find_root_tag(AbstractNode *node, const Location **nextLocation = nullptr);
