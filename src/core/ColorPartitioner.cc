#include "ColorPartitioner.h"
#include "GeometryEvaluator.h"
#include "ColorNode.h"
#include "CsgOpNode.h"
#include "TransformNode.h"
#include "Tree.h"

namespace fs = boost::filesystem;

ColorPartition partition_colors(const AbstractNode & node) {
    LOG(message_group::Echo, "partition_colors: " + node.verbose_name());
    if (auto color_node = dynamic_cast<const ColorNode *>(&node)) {
        return {std::make_pair(color_node->color, node.children)};
    } else if (dynamic_cast<const LeafNode *>(&node)) {
        return {std::make_pair(std::nullopt, NodeVector {node.copy()})};
    } else {
        std::vector<ColorPartition> child_partitions;
        for (const auto & child : node.children) {
            child_partitions.push_back(partition_colors(*child));
        }
        // std::transform(node.children.begin(), node.children.end(), std::back_inserter(child_partitions), partition_colors);
        assert(node.children.size() == child_partitions.size());

        std::set<Color4f> all_colors;
        for (const auto & child_partition : child_partitions) {
            for (const auto & [color, _] : child_partition) {
                if (color.has_value()) {
                    all_colors.insert(color.value());
                }
            }
        }

        auto handle_union = [&]() -> ColorPartition {//std::vector<std::shared_ptr<AbstractNode>> & children) {
            if (all_colors.size() <= 1) {
                std::optional<Color4f> color;
                if (!all_colors.empty()) {
                    color = *all_colors.begin();
                }
                return {std::make_pair(color, node.children)};
            } else 
            {
                ColorPartition result;
                // For each child, for each of its colors, add that child - the others to the result.
                // If it's the first child, also add its intersection w/ the union of the others to its color.
                for (int i = 0, n = child_partitions.size(); i < n; i++) {
                    auto & child_partition = child_partitions[i];
                    auto union_of_others = std::make_shared<CsgOpNode>(new ModuleInstantiation("union"), OpenSCADOperator::UNION);
                    for (int j = 0; j < n; j++) {
                        if (j != i) {
                            // union_of_others->children.push_back(node.children[j]);
                            for (const auto & [other_color, other_nodes] : child_partitions[j]) {
                                for (const auto & other_node : other_nodes) {
                                    union_of_others->children.push_back(other_node->copy());
                                }
                            }
                        }
                    }
                    for (const auto & [color, nodes] : child_partition) {
                        for (const auto & node : nodes) {
                            auto specific_part = std::make_shared<CsgOpNode>(new ModuleInstantiation("difference"), OpenSCADOperator::DIFFERENCE);
                            specific_part->children.push_back(node->copy());
                            specific_part->children.push_back(union_of_others->copy());
                            result[color].push_back(specific_part);

                            if (i == 0) {
                                auto intersection = std::make_shared<CsgOpNode>(new ModuleInstantiation("intersection"), OpenSCADOperator::INTERSECTION);
                                intersection->children.push_back(node->copy());
                                intersection->children.push_back(union_of_others->copy());
                                result[color].push_back(intersection);
                            }
                        }
                    }
                }
                return result;
            }
        };

        if (dynamic_cast<const ListNode *>(&node) || dynamic_cast<const GroupNode *>(&node)) {
            return handle_union();
        }
        if (auto csg_node = dynamic_cast<const CsgOpNode *>(&node)) {
            if (csg_node->type == OpenSCADOperator::UNION) {
                return handle_union();
            } else {
                if (all_colors.size() <= 1) {
                    std::optional<Color4f> color;
                    if (csg_node->type == OpenSCADOperator::DIFFERENCE) {
                        if (!child_partitions[0].empty()) {
                            color = child_partitions[0].begin()->first;
                        }
                    } else {
                        if (!all_colors.empty()) {
                            color = *all_colors.begin();
                        }
                    }
                    return {std::make_pair(color, node.children)};
                }
                ColorPartition result;
                auto & lhs = child_partitions[0];
                for (auto & [color, lhs_nodes] : lhs) {
                    std::shared_ptr<AbstractNode> lhs_node;
                    if (lhs_nodes.size() == 1) {
                        lhs_node = lhs_nodes[0];
                    } else {
                        auto op_node = std::make_shared<CsgOpNode>(new ModuleInstantiation("union"), OpenSCADOperator::UNION);
                        for (auto & lhs_node : lhs_nodes) {
                            lhs_node->children.push_back(lhs_node);
                        }
                        lhs_node = op_node;
                    }
                    auto res = std::make_shared<CsgOpNode>(new ModuleInstantiation(""), csg_node->type);
                    res->children.push_back(lhs_node);
                    for (int i = 1, n = child_partitions.size(); i < n; i++) {
                        for (const auto & [other_color, other_nodes] : child_partitions[i]) {
                            for (const auto & other_node : other_nodes) {
                                res->children.push_back(other_node->copy());
                            }
                        }
                    }
                    result[color].push_back(res);
                }
                return result;
            }
        } else if (auto trans_node = dynamic_cast<const TransformNode *>(&node)) {
            if (all_colors.size() <= 1) {
                std::optional<Color4f> color;
                if (!all_colors.empty()) {
                    color = *all_colors.begin();
                }
                return {std::make_pair(color, node.children)};
            }
            
            ColorPartition result;
            for (const auto & child_partition : child_partitions) {
                for (const auto & [color, child_nodes] : child_partition) {
                    for (const auto & child_node : child_nodes) {
                        result[color].push_back(child_node);
                    }
                }
            }
            for (auto & [color, nodes] : result) {
                auto trans = std::make_shared<TransformNode>(trans_node->modinst, trans_node->verbose_name());
                trans->matrix = trans_node->matrix;
                for (const auto & node : nodes) {
                    trans->children.push_back(node);
                }
                nodes = {trans};
            }
            return result;
        } else {
            LOG(message_group::Error, "Unhandled node type in partition_colors: " + node.verbose_name());
            if (all_colors.size() <= 1) {
                std::optional<Color4f> color;
                if (!all_colors.empty()) {
                    color = *all_colors.begin();
                }
                return {std::make_pair(color, node.children)};
            }
            return {std::make_pair(
                std::nullopt,
                node.children)};
        }
    }
}

std::unique_ptr<Geometry> render_solid_colors(const AbstractNode & node, const fs::path & fparent) {
    auto partition = partition_colors(node);
    std::cout << "Partitioned colors: " << partition.size() << " partitions.\n";
    auto root_node = std::make_shared<RootNode>();
    for (auto& [color, nodes] : partition) {
        auto children_node = std::make_shared<CsgOpNode>(new ModuleInstantiation("union"), OpenSCADOperator::UNION);
        for (const auto& node : nodes) {
            children_node->children.push_back(node);
        }
        if (color.has_value()) {
            auto color_node = std::make_shared<ColorNode>(new ModuleInstantiation("color"));
            color_node->color = color.value();
            color_node->children.push_back(children_node);
            root_node->children.push_back(color_node);
        } else {
            root_node->children.push_back(children_node);
        }
    }
    Tree tree(root_node, fparent.string()); 
    GeometryEvaluator geomevaluator(tree);  

    std::cout << tree.getString(*root_node, "\t") << "\n";

    Geometry::Geometries list;
    for (auto & color_node : root_node->children) {
        auto mesh = geomevaluator.evaluateGeometry(*color_node, /* allownef= */ false);
        if (mesh) {
            list.emplace_back(color_node, mesh);
        }
    }

    return std::make_unique<GeometryList>(list);
}
