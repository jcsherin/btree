#include <gtest/gtest.h>
#include "../src/btree.h"

// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions) {
// Expect two strings not to be equal.
    EXPECT_STRNE("hello", "world");
// Expect equality.
    EXPECT_EQ(7 * 6, 42);

    auto leaf = ElasticNode<int>(5, NodeType::LeafNode);
    EXPECT_FALSE(leaf.IsInternalNode());
    EXPECT_TRUE(leaf.IsLeafNode());
}