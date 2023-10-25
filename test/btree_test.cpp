#include <gtest/gtest.h>
#include "../src/bplustree.h"

using bplustree::NodeType;
using bplustree::ElasticNode;

TEST(BPlusTreeTest, NodeTypes) {
    auto leaf = ElasticNode<int>(NodeType::LeafType, 5);
    EXPECT_TRUE(leaf.GetType() == NodeType::LeafType);

    auto inner = ElasticNode<int>(NodeType::InnerType, 5);
    EXPECT_TRUE(inner.GetType() == NodeType::InnerType);
}
