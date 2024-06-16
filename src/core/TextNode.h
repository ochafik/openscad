#pragma once

#include "node.h"
#include "FreetypeRenderer.h"

class TextModule;

class TextNode : public AbstractPolyNode
{
public:
  VISITABLE();
  TextNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {}

  std::string toString() const override;
  std::string name() const override { return "text"; }
  [[nodiscard]] std::unique_ptr<Geometry> copy() const override {
    auto node = std::make_unique<TextNode>(modinst);
    node->params = params;
    copyChildren(*node.get());
    return node;
  }

  virtual std::vector<std::shared_ptr<const Geometry>> createGeometryList() const;

  virtual FreetypeRenderer::Params get_params() const;

  FreetypeRenderer::Params params;
};
