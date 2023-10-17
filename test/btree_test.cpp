#include <gtest/gtest.h>
#include "../src/btree.h"

// Demonstrate some basic assertions.
TEST(BtreeTest, LeafNodeType) {
    auto leaf = ElasticNode<int>(5, NodeType::LeafNode);
    EXPECT_FALSE(leaf.IsInternalNode());
    EXPECT_TRUE(leaf.IsLeafNode());
}

TEST(BtreeTest, InternalNodeType) {
    auto inner = ElasticNode<int>(5, NodeType::InternalNode);
    EXPECT_FALSE(inner.IsLeafNode());
    EXPECT_TRUE(inner.IsInternalNode());
}
