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

    TEST(BPlusTreeDeleteTest, DeleteNonExistentKey) {
        BPlusTree index{3, 4};

        for (int i = 0; i < 4; ++i) {
            index.Insert(std::make_pair(i, i));

            EXPECT_EQ(index.FindValueOfKey(i), i);
        }

        EXPECT_FALSE(index.Delete(std::make_pair(4, 4)));
    }

    TEST(BPlusTreeDeleteTest, DeleteAllKeys) {
        BPlusTree index{3, 4};

        for (int i = 0; i < 4; ++i) {
            index.Insert(std::make_pair(i, i));
        }
        EXPECT_NE(index.GetRoot(), nullptr);

        for (int i = 0; i < 4; ++i) {
            auto deleted = index.Delete(std::make_pair(i, i));

            EXPECT_TRUE(deleted);
            EXPECT_EQ(index.FindValueOfKey(i), std::nullopt);
        }
        EXPECT_EQ(index.GetRoot(), nullptr);
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

    TEST(BPlusTreeDeleteTest, WithoutLeafUnderflow) {
        BPlusTree index{3, 4};

        std::vector keys{1, 2, 3, 4, 5};
        for (auto &x: keys) {
            index.Insert(std::make_pair(x, x));
        }

        ASSERT_EQ(index.GetRoot()->GetType(), NodeType::InnerType);

        auto root = static_cast<InnerNode *>(index.GetRoot());
        EXPECT_EQ(root->GetCurrentSize(), 1);

        auto leaf1 = static_cast<LeafNode *>(root->GetLowKeyPair().second);
        auto leaf2 = static_cast<LeafNode *> (leaf1->GetSiblingRight());

        EXPECT_EQ(leaf1->GetMinSize(), 2);
        /**
         *          +-------------------+
         *          | Low Key  | (3, *) |
         *          +-------------------+
         *              /             \
         *             /               \
         *            /                 \
         *     +---------------+    +-----------------------+
         *     | (1,1) | (2,2) |    | (3,3) | (4,4) | (5,5) |
         *     +---------------+    +-----------------------+
         *          (leaf1)                 (leaf2)
         *
         *  The leaf node will not underflow as long as it contains
         *  at least 2 elements in this index configuration. So we
         *  can safely remove one value, either 3, 4 or 5 from the
         *  rightmost leaf node without triggering an underflow.
         */

        EXPECT_EQ(leaf1->GetCurrentSize(), 2); // keys: 1, 2
        EXPECT_EQ(leaf2->GetCurrentSize(), 3); // keys: 3, 4, 5

        index.Delete(std::make_pair(4, 4));
        EXPECT_EQ(index.FindValueOfKey(4), std::nullopt);
        EXPECT_EQ(leaf2->GetCurrentSize(), 2);

        std::vector current_keys{keys};
        ASSERT_EQ(current_keys.at(3), 4);
        current_keys.erase(current_keys.begin() + 3); // remove key 4

        int i = 0;
        // Iterate over remaining keys in order - 1, 2, 3, 5
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            ASSERT_EQ((*iter).first, current_keys[i++]);
        }
    }

    TEST(BPlusTreeDeleteTest, BorrowOneFromPreviousLeafNode) {
        auto index = bplustree::BPlusTree(3, 4);

        index.Insert(std::make_pair(1, 1));
        index.Insert(std::make_pair(3, 3));
        index.Insert(std::make_pair(5, 5));
        index.Insert(std::make_pair(7, 7));
        index.Insert(std::make_pair(9, 9));

        index.Insert(std::make_pair(8, 8));
        index.Insert(std::make_pair(6, 6));
        index.Insert(std::make_pair(4, 4));
        index.Insert(std::make_pair(2, 2));

        /**
         *
         * B+Tree structure after the above sequence of insertions.
         *
         *                +---------------------------+
         *                | Low Key | (5, *) | (8, *) |
         *                +---------------------------+
         *                 /           |           \
         *                /            |            \
         *   +---------------+   +-----------+    +-------+
         *   | 1 | 2 | 3 | 4 |   | 5 | 6 | 7 |    | 8 | 9 |
         *   +---------------+   +-----------+    +-------+
         *       (leaf1)            (leaf2)         (leaf3)
         *
         *   Removing a single key, either 8 or 9 from `leaf3` node will
         *   cause it to underflow. It will then borrow one key-value
         *   element from it's previous sibling `leaf2`.
         *
         *   After removing key 8:
         *
         *                +---------------------------+
         *                | Low Key | (5, *) | (7, *) | <-- Transition key updated in parent node for `leaf3`
         *                +---------------------------+
         *                 /           |         \
         *                /            |          \
         *   +---------------+   +-------+    +-------+
         *   | 1 | 2 | 3 | 4 |   | 5 | 6 |    | 7 | 9 |
         *   +---------------+   +-------+    +-------+
         *       (leaf1)          (leaf2)      (leaf3)
         *
         *  The structure of the B+Tree remains the same. But because
         *  we borrowed a key-value element from the previous sibling
         *  the transition key for `leaf3` has to be updated in its
         *  parent node. The borrowed key is 7 and in the parent we
         *  therefore change it from 8 to 7.
         */

        index.Delete(std::make_pair(8, 8));
        EXPECT_EQ(index.FindValueOfKey(8), std::nullopt);

        EXPECT_EQ(index.GetRoot()->GetType(), NodeType::InnerType);
        auto root = static_cast<InnerNode *>(index.GetRoot());
        EXPECT_EQ(root->GetCurrentSize(), 2);

        auto pivot = root->FindPivot(8);
        EXPECT_EQ(pivot->first, 7);

        EXPECT_EQ(pivot->second->GetType(), NodeType::LeafType);
        auto leaf = static_cast<LeafNode *>(pivot->second);

        EXPECT_EQ(leaf->GetCurrentSize(), 2);

        auto previous_leaf = static_cast<LeafNode *>(leaf->GetSiblingLeft());
        EXPECT_EQ(leaf->GetCurrentSize(), 2);

        std::vector keys{1, 2, 3, 4, 5, 6, 7, 9};
        int i = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, keys[i++]);
        }
    }

}
