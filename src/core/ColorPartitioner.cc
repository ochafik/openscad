#include "ColorPartitioner.h"

ColorPartition partition_colors(const AbstractNode & node) {
    if (auto color_node = dynamic_cast<const ColorNode &>(node)) {
        return {std::make_pair(default_color, node.children)};
    } else if (node instanceof LeafNode) {
        return {std::make_pair(std::nullopt, {node.shared_from_this()})};
    } else {
        std::vector<ColorPartition> child_partitions;
        std::transform(csg_node.children.begin(), csg_node.children.end(), std::back_inserter(child_partitions), partition_colors);
        assert(node.children.size() == child_partitions.size());

        std::set<Color4f> all_colors;
        for (const auto & child_partition : child_partitions) {
            for (const auto & [color, _] : child_partition) {
                if (color.has_value()) {
                    all_colors.insert(color.value());
                }
            }
        }

        // if (all_colors.size() <= 1) {
        //     return {
        //         std::make_pair(
        //             all_colors.empty() ? std::nullopt : *all_colors.begin(),
        //             node.children)
        //     };
        // }

        ColorPartition result;
        if (auto csg_node = dynamic_cast<const CsgOpNode &>(node)) {
            switch (csg_node.getOp()) {
                case OpenSCADOperator::UNION:
                    if (all_colors.size() <= 1) {
                        return {std::make_pair(
                            all_colors.empty() ? std::nullopt : *all_colors.begin(),
                            node.children)};
                        // auto & nodes = result[all_colors.empty() ? std::nullopt : *all_colors.begin()];
                        // for (const auto & child_partition : child_partitions) {
                        //     for (const auto & [color, child_nodes] : child_partition) {
                        //         nodes.insert(nodes.end(), child_nodes.begin(), child_nodes.end());
                        //     }
                        }
                    } else 
                    {
                        // For each child, for each of its colors, add that child - the others to the result.
                        // If it's the first child, also add its intersection w/ the union of the others to its color.
                        for (int i = 0, n = child_partitions.size(); i < n; i++) {
                            const auto & child_partition = child_partitions[i];
                            auto union_of_others = std::make_shared<CsgOpNode>(nullptr, OpenSCADOperator::UNION);
                            for (int j = 0; j < n; j++) {
                                if (j != i) {
                                    // union_of_others->addChild(node.children[j]);
                                    for (const auto & [other_color, other_nodes] : child_partition[j]) {
                                        for (const auto & other_node : other_nodes) {
                                            union_of_others->addChild(other_node);
                                        }
                                    }
                                }
                            }
                            for (const auto & [color, nodes] : child_partition) {
                                for (const auto & node : nodes) {
                                    auto specific_part = std::make_shared<CsgOpNode>(nullptr, OpenSCADOperator::DIFFERENCE);
                                    specific_part->addChild(node);
                                    specific_part->addChild(union_of_others);
                                    result[color].push_back(specific_part);

                                    if (i == 0) {
                                        auto intersection = std::make_shared<CsgOpNode>(nullptr, OpenSCADOperator::INTERSECTION);
                                        intersection->addChild(node);
                                        intersection->addChild(union_of_others);
                                        result[color].push_back(intersection);
                                    }
                                }
                            }
                        }
                    }
                    break;
                case OpenSCADOperator::DIFFERENCE:
                    if (child_partitions[0].size() <= 1) {
                        return {std::make_pair(
                            child_partitions[0].empty() ? std::nullopt : child_partitions[0].begin()->first,
                            node.children)};
                    }
                    // fall through next case.
                case OpenSCADOperator::INTERSECTION:
                    if (all_colors.size() <= 1) {
                        return {std::make_pair(
                            all_colors.empty() ? std::nullopt : *all_colors.begin(),
                            node.children)};
                    }
                    auto & lhs = child_partitions[0];
                    for (const auto & [color, lhs_nodes] : lhs) {
                        std::shared_ptr<AbstractNode> lhs_node;
                        if (lhs_nodes.size() == 1) {
                            lhs_node = lhs_nodes[0];
                        } else {
                            lhs_node = std::make_shared<CsgOpNode>(nullptr, OpenSCADOperator::UNION);
                            for (const auto & lhs_node : lhs_nodes) {
                                lhs_node->addChild(lhs_node);
                            }
                        }
                        auto res = std::make_shared<CsgOpNode>(nullptr, csg_node.getOp());
                        res->addChild(lhs_node);
                        for (int i = 1, n = child_partitions.size(); i < n; i++) {
                            for (const auto & [other_color, other_nodes] : child_partitions[i]) {
                                for (const auto & other_node : other_nodes) {
                                    res->addChild(other_node);
                                }
                            }
                        }
                        result[color].push_back(res);
                    }
                    break;
                default:
                    assert(false);
                    break;
            }
        } else if (auto trans_node = dynamic_cast<TransformNode &>(node)) {
            if (all_colors.size() <= 1) {
                return {std::make_pair(
                    all_colors.empty() ? std::nullopt : *all_colors.begin(),
                    node.children)};
            }
            for (const auto & child_partition : child_partitions) {
                for (const auto & [color, child_nodes] : child_partition) {
                    result[color].push_back(child_node);
                }
            }
            for (const auto & [color, nodes] : result) {
                auto trans = std::make_shared<TransformNode>(trans_node.modinst, trans_node.verbose_name());
                trans.matrix = trans_node.matrix;
                for (const auto & node : nodes) {
                    trans.addChild(node);
                }
                nodes = {trans}
            }
        } else {
            if (all_colors.size() <= 1) {
                return {std::make_pair(
                    all_colors.empty() ? std::nullopt : *all_colors.begin(),
                    node.children)};
            }
            return {std::make_pair(
                std::nullopt,
                node.children)};
            // if (auto adv_node = dynamic_cast<CgalAdvNode &>(node)) {
            // switch (adv_node.type) {
            //     case CgalAdvType::MINKOWSKI:
            //     case CgalAdvType::HULL:
            //         // TODO: take lhs color, if any.
            //         break;
            //     case CgalAdvType::FILL:
            //     case CgalAdvType::RESIZE:
            //         auto res = std::make_shared<CgalAdvNode>(adv_node.modinst, adv_node.type);
        }
        return result;
    }
}
