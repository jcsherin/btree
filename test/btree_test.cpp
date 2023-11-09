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

    TEST_F(BPlusTreeTest, IsNotEmptyAfterFirstInsert) {
        index.Insert(std::make_pair(1, 1));

        EXPECT_NE(index.root_, nullptr);
        EXPECT_EQ(index.root_->GetType(), NodeType::LeafType);
        EXPECT_EQ(index.root_->GetMaxSize(), index.leaf_node_max_size_);

        EXPECT_EQ(static_cast<ElasticNode<KeyValuePair> *>(index.root_)->GetCurrentSize(), 1);
    }
}
