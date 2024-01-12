#include <gtest/gtest.h>
#include <numeric>
#include "../src/bplustree.h"
#include <random>

namespace bplustree {
    TEST(BPlusTreeInsertTest, InsertAndFetchEveryKey) {
        BPlusTree index{3, 4};

        std::vector<int> items(10000);
        std::iota(items.begin(), items.end(), 0);

        for (auto &i: items) {
            index.InsertOptimistic(std::make_pair(i, i));

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

    TEST(BPlusTreeInsertTest, InsertInRandomOrder) {
        BPlusTree index{3, 4};

        std::vector<int> items(10000);
        std::iota(items.begin(), items.end(), 0);

        std::random_device rd;
        std::mt19937 g(rd());

        std::shuffle(items.begin(), items.end(), g);

        for (auto &i: items) {
            index.InsertOptimistic(std::make_pair(i, i));

            ASSERT_EQ(index.FindValueOfKey(i), i);
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


    TEST(BPlusTreeInsertTest, AnEmptyTree) {
        BPlusTree index{4, 5};
        EXPECT_EQ(index.GetRoot(), nullptr);
    }
}
