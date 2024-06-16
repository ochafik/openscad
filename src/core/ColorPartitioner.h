#pragma once

#include <map>
#include <optional>
#include <vector>
#include <memory>

#include "node.h"
#include "Geometry.h"
#include <boost/filesystem.hpp>

typedef std::vector<std::shared_ptr<AbstractNode>> NodeVector;
typedef std::map<std::optional<Color4f>, NodeVector> ColorPartition;

ColorPartition partition_colors(AbstractNode & node);

std::unique_ptr<Geometry> evaluate_colors(AbstractNode & node, const boost::filesystem::path & fparent);