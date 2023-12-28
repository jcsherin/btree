#include <gtest/gtest.h>
#include <numeric>
#include "../src/bplustree.h"

namespace bplustree {
    TEST(BPlusTreeInsertTest, InsertAndFetchEveryKey) {
        BPlusTree index{3, 4};

        std::vector<int> items(128);
        std::iota(items.begin(), items.end(), 0);

        for (auto &i: items) {
            index.Insert(std::make_pair(i, i));

            EXPECT_EQ(index.FindValueOfKey(i), i);
        }
        EXPECT_NE(index.GetRoot(), nullptr);

        int i = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, i++);
        }

        int j = items.size() - 1;
        for (auto iter = index.RBegin(); iter != index.REnd(); --iter) {
            EXPECT_EQ((*iter).first, j--);
        }
    }

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

    TEST(BPlusTreeInsertTest, VerifySortedForwardAndBackward) {
        auto index = bplustree::BPlusTree(3, 4);

        int upper_bound = 512;
        std::vector<int> v(upper_bound);
        std::iota(v.begin(), v.end(), 0);

        for (auto &x: v) {
            index.Insert(std::make_pair(x, x));
        }

        for (int i = 0; i < upper_bound; ++i) {
            EXPECT_TRUE(index.FindValueOfKey(i).has_value());
        }

        // Forward Iteration
        int previous_key = -1;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_TRUE((*iter).first > previous_key);
            previous_key = (*iter).first;
        }

        // Backward Iteration
        int next_key = upper_bound;
        for (auto iter = index.RBegin(); iter != index.REnd(); --iter) {
            EXPECT_TRUE((*iter).first < next_key);
            next_key = (*iter).first;
        }
    }

}
