#include <gtest/gtest.h>
#include <numeric>
#include "../src/bplustree.h"

namespace bplustree {
    TEST(BPlusTreeInsertTest, IsEmptyInitially) {
        BPlusTree index{4, 5};
        EXPECT_EQ(index.GetRoot(), nullptr);
    }

    TEST(BPlusTreeInsertTest, AfterFirstInsert) {
        BPlusTree index{4, 5};

        index.Insert(std::make_pair(111, 222));
        ASSERT_NE(index.GetRoot(), nullptr);

        auto root = reinterpret_cast<ElasticNode<KeyValuePair> *>(index.GetRoot());
        EXPECT_EQ(root->GetType(), NodeType::LeafType);
        EXPECT_EQ(root->GetMaxSize(), 5);
        EXPECT_EQ(root->GetCurrentSize(), 1);

        EXPECT_EQ(index.FindValueOfKey(111), 222);
    }
}
