#include <gtest/gtest.h>
#include "../src/bplustree.h"

namespace bplustree {
    class BPlusTreeTest : public testing::Test {
    protected:
        void SetUp() override {}

        BPlusTree index{4, 5};
    };

    TEST_F(BPlusTreeTest, IsEmptyInitially) {
        EXPECT_EQ(index.root_, nullptr);
    }

    TEST_F(BPlusTreeTest, RootIsLeafNode) {
        auto element = std::make_pair(1, 1);
        index.Insert(element);

        auto root_leaf_node = reinterpret_cast<ElasticNode<KeyValuePair> *>(index.root_);

        EXPECT_NE(root_leaf_node, nullptr);
        EXPECT_EQ(root_leaf_node->GetType(), NodeType::LeafType);
        EXPECT_EQ(root_leaf_node->GetMaxSize(), index.leaf_node_max_size_);
        EXPECT_EQ(root_leaf_node->GetCurrentSize(), 1);

        EXPECT_EQ(root_leaf_node->At(0).first, element.first);
        EXPECT_EQ(root_leaf_node->At(0).second, element.second);
    }
}
