// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#pragma once

#include <string>
#include <utility>

#include "node.h"

class RoofNode : public AbstractPolyNode
{
public:
  VISITABLE();
  RoofNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {}
  std::string toString() const override;
  std::string name() const override { return "roof"; }
  [[nodiscard]] std::unique_ptr<AbstractNode> copy() const override {
    auto node = std::make_unique<RoofNode>(modinst);
    node->fa = fa;
    node->fs = fs;
    node->fn = fn;
    node->convexity = convexity;
    node->method = method;
    copyChildren(*node.get());
    return node;
  }

  double fa, fs, fn;
  int convexity = 1;
  std::string method;

  class roof_exception : public std::exception
  {
public:
    roof_exception(std::string message) : m(std::move(message)) {}
    std::string message() {return m;}
private:
    std::string m;
  };
};
