#pragma once

#include <boost/optional.hpp>

#include "node.h"
#include "Value.h"

enum class ImportType {
  UNKNOWN,
  AMF,
  _3MF,
  STL,
  OFF,
  SVG,
  DXF,
  NEF3,
  OBJ,
};

class ImportNode : public LeafNode
{
public:
  constexpr static double SVG_DEFAULT_DPI = 72.0;

  VISITABLE();
  ImportNode(const ModuleInstantiation *mi, ImportType type) : LeafNode(mi), type(type) { }
  std::string toString() const override;
  std::string name() const override;
  [[nodiscard]] std::unique_ptr<AbstractNode> copy() const override {
    auto node = std::make_unique<ImportNode>(modinst, type);
    node->type = type;
    node->filename = filename;
    node->id = id;
    node->layer = layer;
    node->convexity = convexity;
    node->center = center;
    node->dpi = dpi;
    node->fn = fn;
    node->fs = fs;
    node->fa = fa;
    node->origin_x = origin_x;
    node->origin_y = origin_y;
    node->scale = scale;
    node->width = width;
    node->height = height;
    copyChildren(*node.get());
    return node;
  }

  ImportType type;
  Filename filename;
  boost::optional<std::string> id;
  boost::optional<std::string> layer;
  int convexity;
  bool center;
  double dpi;
  double fn, fs, fa;
  double origin_x, origin_y, scale;
  double width, height;
  std::unique_ptr<const class Geometry> createGeometry() const override;
};
