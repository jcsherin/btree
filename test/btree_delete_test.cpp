#include <gtest/gtest.h>
#include <numeric>
#include "../src/bplustree.h"

namespace bplustree {
    TEST(BPlusTreeDeleteTest, SimpleDelete) {
        BPlusTree index{3, 4};

        for (int i = 0; i < 4; ++i) {
            index.Insert(std::make_pair(i, i));
            EXPECT_EQ(index.FindValueOfKey(i), i);
        }

        auto root = static_cast<ElasticNode<KeyValuePair> *>(index.GetRoot());
        EXPECT_NE(root, nullptr);

        for (int i = 0; i < 4; ++i) {
            auto deleted = index.Delete(std::make_pair(i, i));
            EXPECT_TRUE(deleted);
        }

        auto new_root = static_cast<ElasticNode<KeyValuePair> *>(index.GetRoot());
        EXPECT_EQ(new_root, nullptr);
    }
}
