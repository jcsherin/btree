#include <gtest/gtest.h>
#include <numeric>
#include "../src/bplustree.h"

namespace bplustree {
    TEST(BPlusTreeDeleteTest, SimpleTest) {
        BPlusTree index{3, 4};

        index.Insert(std::make_pair(1, 1));
        EXPECT_EQ(index.FindValueOfKey(1), 1);

        auto root = static_cast<ElasticNode<KeyValuePair> *>(index.GetRoot());
        EXPECT_NE(root, nullptr);

        auto deleted = index.Delete(std::make_pair(1, 1));
        EXPECT_TRUE(deleted);
    }
}
