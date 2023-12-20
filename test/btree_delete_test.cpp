#include <gtest/gtest.h>
#include <numeric>
#include "../src/bplustree.h"

namespace bplustree {
    TEST(BPlusTreeDeleteTest, DeleteAnExistingKey) {
        BPlusTree index{3, 4};

        for (int i = 0; i < 4; ++i) {
            index.Insert(std::make_pair(i, i));

            EXPECT_EQ(index.FindValueOfKey(i), i);
        }

        for (int i = 0; i < 4; ++i) {
            auto deleted = index.Delete(std::make_pair(i, i));

            EXPECT_TRUE(deleted);
            EXPECT_EQ(index.FindValueOfKey(i), std::nullopt);
        }

    }

    TEST(BPlusTreeDeleteTest, RootUnderflowAllowed) {
        BPlusTree index{3, 4};

        index.Insert(std::make_pair(1, 1));
        index.Insert(std::make_pair(2, 2));

        ASSERT_EQ(index.GetRoot()->GetType(), NodeType::LeafType);

        auto root = static_cast<LeafNode *>(index.GetRoot());
        ASSERT_EQ(root->GetCurrentSize(), 2);
        ASSERT_EQ(root->GetMinSize(), 2);

        index.Delete(std::make_pair(1, 1));
        EXPECT_EQ(index.FindValueOfKey(1), std::nullopt);

        ASSERT_EQ(root->GetCurrentSize(), 1);
        EXPECT_EQ(root, index.GetRoot());
    }
}
