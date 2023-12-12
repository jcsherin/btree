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

    TEST(BPlusTreeInsertTest, FillRootNode) {
        BPlusTree index{3, 4};
        index.Insert(std::make_pair(1, 1));

        index.Insert(std::make_pair(2, 2));
        index.Insert(std::make_pair(3, 3));
        index.Insert(std::make_pair(4, 4));

        auto root = static_cast<ElasticNode<KeyValuePair> *>(index.GetRoot());
        EXPECT_EQ(root->GetType(), NodeType::LeafType);
        EXPECT_EQ(root->GetMaxSize(), 4);
        EXPECT_EQ(root->GetCurrentSize(), 4);

        EXPECT_EQ(index.FindValueOfKey(1), 1);
        EXPECT_EQ(index.FindValueOfKey(2), 2);
        EXPECT_EQ(index.FindValueOfKey(3), 3);
        EXPECT_EQ(index.FindValueOfKey(4), 4);
    }

    TEST(BPlusTreeInsertTest, WillNotFitInSingleNode) {
        BPlusTree index{3, 4};
        index.Insert(std::make_pair(1, 1));

        index.Insert(std::make_pair(2, 2));
        index.Insert(std::make_pair(3, 3));
        index.Insert(std::make_pair(4, 4));
        index.Insert(std::make_pair(5, 5));

        auto root = static_cast<ElasticNode<KeyValuePair> *>(index.GetRoot());
        EXPECT_EQ(root->GetType(), NodeType::InnerType);

        EXPECT_EQ(index.FindValueOfKey(1), 1);
        EXPECT_EQ(index.FindValueOfKey(2), 2);
        EXPECT_EQ(index.FindValueOfKey(3), 3);
        EXPECT_EQ(index.FindValueOfKey(4), 4);
        EXPECT_EQ(index.FindValueOfKey(5), 5);
    }
}
