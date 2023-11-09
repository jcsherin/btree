#include <gtest/gtest.h>
#include "../src/bplustree.h"

namespace bplustree {
    TEST(BPlusTreeTest, NodeTypes) {
        auto leaf = ElasticNode<int>(NodeType::LeafType, std::make_pair(0, nullptr), 5);
        EXPECT_TRUE(leaf.GetType() == NodeType::LeafType);

        auto inner = ElasticNode<int>(NodeType::InnerType, std::make_pair(0, nullptr), 5);
        EXPECT_TRUE(inner.GetType() == NodeType::InnerType);
    }
}
