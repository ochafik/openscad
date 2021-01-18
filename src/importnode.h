#pragma once

#include "node.h"
#include "value.h"

enum class ImportType {
	UNKNOWN,
	AMF,
	_3MF,
	STL,
	OFF,
	SVG,
	DXF,
	NEF3,
};

class ImportNode : public LeafNode
{
public:
	constexpr static double SVG_DEFAULT_DPI = 72.0;

	VISITABLE();
	ImportNode(const ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx, ImportType type) : LeafNode(mi, ctx), type(type) { }
	std::string toString() const override;
	std::string name() const override;

	ImportType type;
	Filename filename;
	std::string layername;
	int convexity;
	bool center;
	double dpi;
	double fn, fs, fa;
	double origin_x, origin_y, scale;
	double width, height;
	const class Geometry *createGeometry() const override;

protected:
  int getDimensionImpl() const override {
    switch (this->type) {
      case ImportType::STL:
      case ImportType::AMF:
      case ImportType::_3MF:
      case ImportType::OFF:
      case ImportType::NEF3:
        return 3;
      case ImportType::SVG:
      case ImportType::DXF:
        return 2;
      case ImportType::UNKNOWN:
        return 0;
      default:
        assert(false);
        return 0;
    }
  }
};
