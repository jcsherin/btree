#include <gtest/gtest.h>
#include <numeric>
#include "../src/bplustree.h"

namespace bplustree {
    TEST(BPlusTreeDeleteTest, DeleteAnExistingKey) {
        BPlusTree index{3, 4};

        // Empty B+Tree
        EXPECT_EQ(index.GetRoot(), nullptr);

        for (int i = 0; i < 4; ++i) {
            index.Insert(std::make_pair(i, i));

            EXPECT_EQ(index.FindValueOfKey(i), i);
        }

        EXPECT_NE(index.GetRoot(), nullptr);

        for (int i = 0; i < 4; ++i) {
            auto deleted = index.Delete(std::make_pair(i, i));

            EXPECT_TRUE(deleted);
            EXPECT_EQ(index.FindValueOfKey(i), std::nullopt);
        }

        EXPECT_EQ(index.GetRoot(), nullptr);
    }
}
