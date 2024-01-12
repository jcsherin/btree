#include <gtest/gtest.h>
#include <numeric>
#include <random>
#include "../src/bplustree.h"

namespace bplustree {
    TEST(BPlusTreeDeleteTest, DeleteNonExistentKey) {
        BPlusTree index{3, 4};

        for (int i = 0; i < 4; ++i) {
            index.InsertOptimistic(std::make_pair(i, i));

            EXPECT_EQ(index.FindValueOfKey(i), i);
        }

        EXPECT_FALSE(index.Delete(4));
    }

    TEST(BPlusTreeDeleteTest, DeleteEveryKey) {
        BPlusTree index{3, 4};

        int count = 128;

        for (int i = 0; i < count; ++i) {
            index.InsertOptimistic(std::make_pair(i, i));

            EXPECT_EQ(index.FindValueOfKey(i), i);
        }
        EXPECT_NE(index.GetRoot(), nullptr);

        for (int i = 0; i < count; ++i) {
            auto deleted = index.Delete(i);

            EXPECT_TRUE(deleted);
            EXPECT_EQ(index.FindValueOfKey(i), std::nullopt);
        }
        EXPECT_EQ(index.GetRoot(), nullptr);
    }

    TEST(BPlusTreeDeleteTest, DeleteEveryKeyInRandomOrder) {
        BPlusTree index{3, 4};

        std::vector<int> items(512);
        std::iota(items.begin(), items.end(), 0);

        for (auto &i: items) {
            index.InsertOptimistic(std::make_pair(i, i));

            EXPECT_EQ(index.FindValueOfKey(i), i);
        }
        EXPECT_NE(index.GetRoot(), nullptr);

        std::random_device rd;
        std::mt19937 g(rd());

        std::shuffle(items.begin(), items.end(), g);
        for (auto &i: items) {
            auto deleted = index.Delete(i);

            EXPECT_TRUE(deleted);
            EXPECT_EQ(index.FindValueOfKey(i), std::nullopt);
        }

        EXPECT_EQ(index.GetRoot(), nullptr);
    }

    TEST(BPlusTreeDeleteTest, RootUnderflowAllowed) {
        BPlusTree index{3, 4};

        index.InsertOptimistic(std::make_pair(1, 1));
        index.InsertOptimistic(std::make_pair(2, 2));

        ASSERT_EQ(index.GetRoot()->GetType(), NodeType::LeafType);

        auto root = static_cast<LeafNode *>(index.GetRoot());
        ASSERT_EQ(root->GetCurrentSize(), 2);
        ASSERT_EQ(root->GetMinSize(), 2);

        index.Delete(1);
        EXPECT_EQ(index.FindValueOfKey(1), std::nullopt);

        ASSERT_EQ(root->GetCurrentSize(), 1);
        EXPECT_EQ(root, index.GetRoot());
    }

    TEST(BPlusTreeDeleteTest, WithoutLeafUnderflow) {
        BPlusTree index{3, 4};

        std::vector keys{1, 2, 3, 4, 5};
        for (auto &x: keys) {
            index.InsertOptimistic(std::make_pair(x, x));
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

        index.Delete(4);
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

        index.InsertOptimistic(std::make_pair(1, 1));
        index.InsertOptimistic(std::make_pair(3, 3));
        index.InsertOptimistic(std::make_pair(5, 5));
        index.InsertOptimistic(std::make_pair(7, 7));
        index.InsertOptimistic(std::make_pair(9, 9));

        index.InsertOptimistic(std::make_pair(8, 8));
        index.InsertOptimistic(std::make_pair(6, 6));
        index.InsertOptimistic(std::make_pair(4, 4));
        index.InsertOptimistic(std::make_pair(2, 2));

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

        index.Delete( 8);
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

    TEST(BPlusTreeDeleteTest, MergeWithPreviousLeafNode) {

        auto index = bplustree::BPlusTree(3, 4);

        index.InsertOptimistic(std::make_pair(1, 1));
        index.InsertOptimistic(std::make_pair(3, 3));
        index.InsertOptimistic(std::make_pair(5, 5));
        index.InsertOptimistic(std::make_pair(7, 7));
        index.InsertOptimistic(std::make_pair(9, 9));

        index.InsertOptimistic(std::make_pair(8, 8));
        index.InsertOptimistic(std::make_pair(6, 6));
        index.InsertOptimistic(std::make_pair(4, 4));
        index.InsertOptimistic(std::make_pair(2, 2));

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
         *
         *   Removing one key from `leaf3` and it will borrow one
         *   from `leaf2` to prevent underflow. Remove one more key
         *   again from `leaf3` and it cannot borrow from `leaf2`
         *   anymore because that will cause in an underflow in
         *   `leaf2`. So the only option here is for `leaf3` to
         *   merge with `leaf2`.
         *
         *   After removing key 8, then 7:
         *
         *                +------------------+
         *                | Low Key | (5, *) | <-- Transition key to `leaf3` removed from parent
         *                +------------------+
         *                 /             |
         *                /              |
         *   +---------------+   +-----------+
         *   | 1 | 2 | 3 | 4 |   | 5 | 6 | 9 |
         *   +---------------+   +-----------+
         *       (leaf1)          (leaf2)
         *
         *  Since `leaf3` doesn't exist anymore its pivot element in
         *  parent node is also removed.
         */

        index.Delete( 8);
        EXPECT_EQ(index.FindValueOfKey(8), std::nullopt);

        index.Delete( 7);
        EXPECT_EQ(index.FindValueOfKey(7), std::nullopt);

        EXPECT_EQ(index.GetRoot()->GetType(), NodeType::InnerType);
        auto root = static_cast<InnerNode *>(index.GetRoot());
        EXPECT_EQ(root->GetCurrentSize(), 1);

        auto leaf2 = static_cast<LeafNode *>(root->FindPivot(9)->second);
        EXPECT_EQ(leaf2->GetSiblingRight(), nullptr);
        EXPECT_EQ(leaf2->GetCurrentSize(), 3);

        std::vector keys{1, 2, 3, 4, 5, 6, 9};
        int i = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, keys[i++]);
        }
    }

    TEST(BPlusTreeDeleteTest, BorrowOneFromNextLeafNode) {
        auto index = bplustree::BPlusTree(3, 4);

        index.InsertOptimistic(std::make_pair(1, 1));
        index.InsertOptimistic(std::make_pair(3, 3));
        index.InsertOptimistic(std::make_pair(5, 5));
        index.InsertOptimistic(std::make_pair(7, 7));
        index.InsertOptimistic(std::make_pair(9, 9));
        index.InsertOptimistic(std::make_pair(11, 11));


        /**
         *
         * B+Tree structure after the above sequence of insertions.
         *
         *                +------------------+
         *                | Low Key | (5, *) |
         *                +------------------+
         *                 /           |
         *                /            |
         *           +-------+   +----------------+
         *           | 1 | 3 |   | 5 | 7 | 9 | 11 |
         *           +-------+   +----------------+
         *            (leaf1)     (leaf2)
         *
         *   Removing a single key, either 1  or 3 from `leaf1` will
         *   cause underflow. This is rectified by borrowing one element
         *   from the next sibling leaf node `leaf2`.
         *
         *   After removing key 1:
         *
         *                +------------------+
         *                | Low Key | (7, *) | <-- Transition key updated to 7
         *                +------------------+
         *                 /           |
         *                /            |
         *           +-------+   +------------+
         *           | 3 | 5 |   | 7 | 9 | 11 |
         *           +-------+   +------------+
         *            (leaf1)     (leaf2)
         *
         *   The element corresponding to key: 5 is borrowed from `leaf2`
         *   in to `leaf1`. The transition key for `leaf2` is also updated
         *   in the parent node.
         */

        index.Delete( 1);
        EXPECT_EQ(index.FindValueOfKey(1), std::nullopt);

        EXPECT_EQ(index.GetRoot()->GetType(), NodeType::InnerType);
        auto root = static_cast<InnerNode *>(index.GetRoot());
        EXPECT_EQ(root->GetCurrentSize(), 1);

        auto pivot = root->FindPivot(9);
        EXPECT_EQ(pivot->first, 7);

        auto leaf1 = static_cast<LeafNode *>(root->GetLowKeyPair().second);
        EXPECT_EQ(leaf1->GetCurrentSize(), 2);

        auto leaf2 = static_cast<LeafNode *>(leaf1->GetSiblingRight());
        EXPECT_EQ(leaf2->GetCurrentSize(), 3);

        std::vector keys{3, 5, 7, 9, 11};
        int i = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, keys[i++]);
        }
    }

    TEST(BPlusTreeDeleteTest, MergeWithNextLeafNode) {

        auto index = bplustree::BPlusTree(3, 4);

        index.InsertOptimistic(std::make_pair(1, 1));
        index.InsertOptimistic(std::make_pair(3, 3));
        index.InsertOptimistic(std::make_pair(5, 5));
        index.InsertOptimistic(std::make_pair(7, 7));
        index.InsertOptimistic(std::make_pair(9, 9));
        index.InsertOptimistic(std::make_pair(11, 11));
        index.InsertOptimistic(std::make_pair(13, 13));

        /**
         *
         * B+Tree structure after the above sequence of insertions.
         *
         *                +---------------------------+
         *                | Low Key | (5, *) | (9, *) |
         *                +---------------------------+
         *                 /           |           \
         *                /            |            \
         *          +-------+       +-------+    +-------------+
         *          | 1 | 3 |       | 5 | 7 |    | 9 | 11 | 13 |
         *          +-------+       +-------+    +-------------+
         *           (leaf1)         (leaf2)         (leaf3)
         *
         *
         * Removing one key from `leaf1` either 1 or 3 will cause
         * it to underflow then merge with `leaf2`.
         *
         * After removing key 1:
         *
         *                +------------------+
         *                | Low Key | (9, *) | <-- Transition key to `leaf2` removed
         *                +------------------+
         *                 /             |
         *                /              |
         *          +-----------+    +-------------+
         *          | 3 | 5 | 7 |    | 9 | 11 | 13 |
         *          +-----------+    +-------------+
         *
         *  On merging `leaf1` and `leaf3` in the original B+Tree
         *  one of the node pointers in the parent node has to be
         *  removed. Here the transition key 5 and the corresponding
         *  node pointer is removed from the parent node.
         */

        index.Delete( 1);
        EXPECT_EQ(index.FindValueOfKey(1), std::nullopt);

        EXPECT_EQ(index.GetRoot()->GetType(), NodeType::InnerType);
        auto root = static_cast<InnerNode *>(index.GetRoot());
        EXPECT_EQ(root->GetCurrentSize(), 1);

        auto leaf1 = static_cast<LeafNode *>(root->GetLowKeyPair().second);
        EXPECT_EQ(leaf1->GetCurrentSize(), 3);

        std::vector keys{3, 5, 7, 9, 11, 13};
        int i = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, keys[i++]);
        }
    }

    TEST(BPlusTreeDeleteTest, BorrowOneFromNextInnerNode) {
        auto index = bplustree::BPlusTree(3, 3);

        std::vector insert_keys{3, 6, 9, 12, 15, 18, 21, 27, 33, 39, 45};
        for (auto &x: insert_keys) {
            index.InsertOptimistic(std::make_pair(x, x));
        }

        /**
         * B+Tree after insertions:
         *
         *                   +--------------+
         *                   | * | (15, * ) |                              <-- Root
         *                   +--------------+
         *                    /          \
         *        +------------+         +-----------------------+
         *        | * | (9, *) |         | * | (21, *) | (33, *) |         <-- Inner  Nodes
         *        +------------+         +-----------------------+
         *         /        \            /          |         \
         *      +---+     +----+     +-----+     +-----+     +--------+
         *      |3|6|<--->|9|12|<--->|15|18|<--->|21|27|<--->|33|39|45|    <-- Leaf Nodes
         *      +---+     +----+     +-----+     +-----+     +--------+
         *
         *                   +----------------------+
         *                   | Keys | Fanout |  Min |
         *      +------------+----------------------+
         *      | Inner Node |   3  |     4  |    1 |
         *      +-----------------------------------+
         *      | Leaf Node  |   3  |  n/a   |   2  |
         *      +-----------------------------------+
         *
         * Deleting the element with key `9` will cause the first inner node to
         * underflow when the pivot element `(9, *)` is removed. The `*` here
         * represents a node pointer. The inner node should contain a minimum
         * of 1 key and 2 node pointers. So it will borrow one element from
         * the next inner node to fix underflow.
         *
         * The pivot element between these two inner nodes will also need to
         * be updated for search/traversal to continue to work correctly
         * by updating the key.
         */

        // Verify preconditions
        EXPECT_EQ(index.GetRoot()->GetType(), NodeType::InnerType);
        auto root = static_cast<InnerNode *>(index.GetRoot());

        EXPECT_EQ(root->GetCurrentSize(), 1);
        EXPECT_EQ(root->GetLowKeyPair().second->GetType(), NodeType::InnerType);
        EXPECT_EQ(root->Begin()->second->GetType(), NodeType::InnerType);

        auto pivot = root->Begin();
        EXPECT_EQ(pivot->first, 15);

        auto inner = static_cast<InnerNode *>(root->GetLowKeyPair().second);
        EXPECT_EQ(inner->GetCurrentSize(), 1);
        EXPECT_EQ(inner->Begin()->first, 9);
        EXPECT_EQ(inner->Begin()->second->GetType(), NodeType::LeafType);

        auto next_inner = static_cast<InnerNode *>(pivot->second);
        EXPECT_EQ(next_inner->GetCurrentSize(), 2);
        EXPECT_EQ(next_inner->Begin()->first, 21);
        EXPECT_EQ(next_inner->Begin()->second->GetType(), NodeType::LeafType);

        // Trigger borrow from next inner node
        index.Delete( 9);

        /**
         * B+Tree after borrowing from next inner node:
         *
         *                   +--------------+
         *                   | * | (21, * ) |
         *                   +--------------+
         *                    /           \
         *        +------------+         +--------------+
         *        | * | (15, *) |         | * | (33, *) |
         *        +------------+         +--------------+
         *         /          \             |         \
         *      +------+     +-----+     +-----+     +--------+
         *      |3|6|12|<--->|15|18|<--->|21|27|<--->|33|39|45|
         *      +------+     +-----+     +-----+     +--------+
         */

        // Verify post conditions
        EXPECT_EQ(root->GetCurrentSize(), 1);
        EXPECT_EQ(root->GetLowKeyPair().second->GetType(), NodeType::InnerType);
        EXPECT_EQ(root->Begin()->second->GetType(), NodeType::InnerType);

        EXPECT_EQ(pivot->first, 21);

        EXPECT_EQ(inner->GetCurrentSize(), 1);
        EXPECT_EQ(inner->Begin()->first, 15);
        EXPECT_EQ(inner->Begin()->second->GetType(), NodeType::LeafType);

        EXPECT_EQ(next_inner->GetCurrentSize(), 1);
        EXPECT_EQ(next_inner->Begin()->first, 33);
        EXPECT_EQ(next_inner->Begin()->second->GetType(), NodeType::LeafType);


        std::vector remaining_keys{3, 6, 12, 15, 18, 21, 27, 33, 39, 45};

        // Verify forward iteration
        int i = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, remaining_keys[i++]);
        }

        // Verify backward iteration
        int j = remaining_keys.size() - 1;
        for (auto iter = index.RBegin(); iter != index.REnd(); --iter) {
            EXPECT_EQ((*iter).first, remaining_keys[j--]);
        }
    }

    TEST(BPlusTreeDeleteTest, MergeWithNextInnerNode) {
        auto index = bplustree::BPlusTree(3, 3);

        std::vector insert_keys{3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42};
        for (auto &x: insert_keys) {
            index.InsertOptimistic(std::make_pair(x, x));
        }

        /**
         * B+Tree after insertions:
         *
         *             +--------------------------------+
         *             | * |    (15, * ) |      (27, *) |
         *             +--------------------------------+
         *              /            |                \
         *    +----------+       +-----------+       +------------------------+
         *    | * |(9, *)|       | * |(21, *)|       | * | (33, * ) | (39, *) |
         *    +----------+       +-----------+       +------------------------+
         *    /        \        |        |            /         |          \
         * +---+    +----+    +-----+    +-----+    +-----+    +-----+    +-----+
         * |3|6|<-->|9|12|<-->|15|18|<-->|21|24|<-->|27|30|<-->|33|36|<-->|39|42|
         * +---+    +----+    +-----+    +-----+    +-----+    +-----+    +-----+
         *
         *                   +----------------------+
         *                   | Keys | Fanout |  Min |
         *      +------------+----------------------+
         *      | Inner Node |   3  |     4  |    1 |
         *      +-----------------------------------+
         *      | Leaf Node  |   3  |  n/a   |   2  |
         *      +-----------------------------------+
         *
         * Deleting the element with key `9` will cause the first inner node
         * to underflow when the pivot element `(9, *)` is removed. The `*`
         * represents a node pointer. The inner node should contain a minimum
         * of 1 key and 2 node pointers, so it will merge with the next inner
         * node to re-balance the tree.
         *
         * After the merge the first inner node is deleted. Its reference in
         * the parent(root) node also needs to be removed. Here in this case
         * it is the lowest node pointer, and that is instead replaced with
         * a reference to the next inner node which merged.
         */

        // Verify preconditions
        EXPECT_EQ(index.GetRoot()->GetType(), NodeType::InnerType);
        auto root = static_cast<InnerNode *>(index.GetRoot());

        EXPECT_EQ(root->GetCurrentSize(), 2);
        EXPECT_EQ(root->GetLowKeyPair().second->GetType(), NodeType::InnerType);
        EXPECT_EQ(root->Begin()->second->GetType(), NodeType::InnerType);

        auto pivot = root->Begin();
        EXPECT_EQ(pivot->first, 15);

        auto inner = static_cast<InnerNode *>(root->GetLowKeyPair().second);
        EXPECT_EQ(inner->GetCurrentSize(), 1);
        EXPECT_EQ(inner->Begin()->first, 9);
        EXPECT_EQ(inner->Begin()->second->GetType(), NodeType::LeafType);

        auto next_inner = static_cast<InnerNode *>(pivot->second);
        EXPECT_EQ(next_inner->GetCurrentSize(), 1);
        EXPECT_EQ(next_inner->Begin()->first, 21);
        EXPECT_EQ(next_inner->Begin()->second->GetType(), NodeType::LeafType);

        // Trigger borrow from next inner node
        index.Delete( 9);

        /**
         * B+Tree after merge with next inner node:
         *
         *                          +-------------+
         *                          | * | (27, *) |
         *                          +-------------+
         *                          /            \
         *    +---------------------+           +------------------------+
         *    | * | (15, *) | (21,*) |           | * | (33, * ) | (39, *) |
         *    +---------------------+           +------------------------+
         *    /          \          \            /         |           \
         * +------+    +-----+    +-----+    +-----+    +-----+    +-----+
         * |3|6|12|<-->|15|18|<-->|21|24|<-->|27|30|<-->|33|36|<-->|39|42|
         * +------+    +-----+    +-----+    +-----+    +-----+    +-----+
         *
         */
        // Verify post conditions
        EXPECT_EQ(root->GetCurrentSize(), 1);
        EXPECT_EQ(root->GetLowKeyPair().second->GetType(), NodeType::InnerType);
        EXPECT_EQ(root->Begin()->second->GetType(), NodeType::InnerType);

        EXPECT_EQ(pivot->first, 27);

        EXPECT_EQ(inner->GetCurrentSize(), 2);
        EXPECT_EQ(inner->Begin()->first, 15);
        EXPECT_EQ(inner->Begin()->second->GetType(), NodeType::LeafType);
        EXPECT_EQ(inner->RBegin()->first, 21);

        EXPECT_EQ(index.FindValueOfKey(9), std::nullopt);

        std::vector remaining_keys{3, 6, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42};
        // Verify forward iteration
        int i = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, remaining_keys[i++]);
        }

        // Verify backward iteration
        int j = remaining_keys.size() - 1;
        for (auto iter = index.RBegin(); iter != index.REnd(); --iter) {
            EXPECT_EQ((*iter).first, remaining_keys[j--]);
        }
    }

    TEST(BPlusTreeDeleteTest, BorrowOneFromPreviousInnerNode) {
        auto index = bplustree::BPlusTree(3, 3);

        std::vector insert_keys{3, 6, 9, 12, 15, 18, 21, 24, 4, 5, 7, 8, 10};
        for (auto &x: insert_keys) {
            index.InsertOptimistic(std::make_pair(x, x));
        }

        /**
         * B+Tree after insertions:
         *
         *                             +--------------+
         *                             | * | (15, * ) |
         *                             +--------------+
         *                              /           \
         *    +----------------------------+       +-------------+
         *    | * | (5, *) | (7,*) | (9,*) |       | * | (21, *) |
         *    +----------------------------+       +-------------+
         *    /        \        |        |            /        \
         * +---+    +---+    +---+    +-------+    +-----+    +-----+
         * |3|4|<-->|5|6|<-->|7|8|<-->|9|10|12|<-->|15|18|<-->|21|24|
         * +---+    +---+    +---+    +-------+    +-----+    +-----+
         *
         *                   +----------------------+
         *                   | Keys | Fanout |  Min |
         *      +------------+----------------------+
         *      | Inner Node |   3  |     4  |    1 |
         *      +-----------------------------------+
         *      | Leaf Node  |   3  |  n/a   |   2  |
         *      +-----------------------------------+
         *
         * Deleting the element with key `21` will cause the second inner node
         * to underflow when the pivot element `(21, *)` is removed. The `*`
         * represents a node pointer. The inner should contain a minimum of
         * of 1 key and 2 node pointers, so here it will borrow 1 node pointer
         * from the previous inner node to re-balance the tree.
         *
         * The pivot element key in the parent(root) node is updated so that
         * traversal can continue to function correctly after the borrow is
         * completed and the tree is re-balanced.
         */

        // Verify preconditions
        EXPECT_EQ(index.GetRoot()->GetType(), NodeType::InnerType);
        auto root = static_cast<InnerNode *>(index.GetRoot());

        EXPECT_EQ(root->GetCurrentSize(), 1);
        EXPECT_EQ(root->GetLowKeyPair().second->GetType(), NodeType::InnerType);
        EXPECT_EQ(root->Begin()->second->GetType(), NodeType::InnerType);

        auto pivot = root->Begin();
        EXPECT_EQ(pivot->first, 15);

        auto inner = static_cast<InnerNode *>(pivot->second);
        EXPECT_EQ(inner->GetCurrentSize(), 1);
        EXPECT_EQ(inner->Begin()->first, 21);
        EXPECT_EQ(inner->Begin()->second->GetType(), NodeType::LeafType);

        auto prev_inner = static_cast<InnerNode *>(root->GetLowKeyPair().second);
        EXPECT_EQ(prev_inner->GetCurrentSize(), 3);
        EXPECT_EQ(prev_inner->Begin()->first, 5);
        EXPECT_EQ(prev_inner->Begin()->second->GetType(), NodeType::LeafType);


        // Trigger borrow from previous inner node
        index.Delete( 21);

        /**
         * B+Tree after borrow from previous inner node:
         *
         *                      +--------------+
         *                      | * | (9, * ) |
         *                      +--------------+
         *                      /           \
         *    +--------------------+       +-------------+
         *    | * | (5, *) | (7,*) |       | * | (15, *) |
         *    +--------------------+       +-------------+
         *    /        \        |           /          \
         * +---+    +---+    +---+    +-------+    +--------+
         * |3|4|<-->|5|6|<-->|7|8|<-->|9|10|12|<-->|15|18|24|
         * +---+    +---+    +---+    +-------+    +--------+
         *
         */

        // Verify post conditions
        EXPECT_EQ(root->GetCurrentSize(), 1);
        EXPECT_EQ(root->GetLowKeyPair().second->GetType(), NodeType::InnerType);
        EXPECT_EQ(root->Begin()->second->GetType(), NodeType::InnerType);

        EXPECT_EQ(pivot->first, 9);

        EXPECT_EQ(inner->GetCurrentSize(), 1);
        EXPECT_EQ(inner->Begin()->first, 15);
        EXPECT_EQ(inner->Begin()->second->GetType(), NodeType::LeafType);

        EXPECT_EQ(prev_inner->GetCurrentSize(), 2);
        EXPECT_EQ(prev_inner->Begin()->first, 5);
        EXPECT_EQ(prev_inner->Begin()->second->GetType(), NodeType::LeafType);

        EXPECT_EQ(index.FindValueOfKey(21), std::nullopt);

        std::vector remaining_keys{3, 4, 5, 6, 7, 8, 9, 10, 12, 15, 18, 24};
        // Verify forward iteration
        int i = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, remaining_keys[i++]);
        }

        // Verify backward iteration
        int j = remaining_keys.size() - 1;
        for (auto iter = index.RBegin(); iter != index.REnd(); --iter) {
            EXPECT_EQ((*iter).first, remaining_keys[j--]);
        }
    }

    TEST(BPlusTreeDeleteTest, MergeWithPreviousInnerNode) {
        auto index = bplustree::BPlusTree(3, 3);

        std::vector insert_keys{3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42};
        for (auto &x: insert_keys) {
            index.InsertOptimistic(std::make_pair(x, x));
        }

        /**
         * B+Tree after insertions:
         *
         *             +--------------------------------+
         *             | * |    (15, * ) |      (27, *) |
         *             +--------------------------------+
         *              /            |                \
         *    +----------+       +-----------+       +------------------------+
         *    | * |(9, *)|       | * |(21, *)|       | * | (33, * ) | (39, *) |
         *    +----------+       +-----------+       +------------------------+
         *    /        \        |        |            /         |          \
         * +---+    +----+    +-----+    +-----+    +-----+    +-----+    +-----+
         * |3|6|<-->|9|12|<-->|15|18|<-->|21|24|<-->|27|30|<-->|33|36|<-->|39|42|
         * +---+    +----+    +-----+    +-----+    +-----+    +-----+    +-----+
         *
         *                   +----------------------+
         *                   | Keys | Fanout |  Min |
         *      +------------+----------------------+
         *      | Inner Node |   3  |     4  |    1 |
         *      +-----------------------------------+
         *      | Leaf Node  |   3  |  n/a   |   2  |
         *      +-----------------------------------+
         *
         * Deleting the element with key `21` will cause the second(middle)
         * inner node to underflow when the pivot element `(21, *)` is
         * removed. The `*` represents a node pointer. The inner node should
         * contain a minimum of 1 key and 2 node pointers, so it will merge
         * with the previous inner node to re-balance the tree.
         *
         * After the merge the middle node does not exist in the tree. So its
         * reference in parent(root) node, the pivot element `(15, *)` is also
         * removed.
         */

        // Verify preconditions
        EXPECT_EQ(index.GetRoot()->GetType(), NodeType::InnerType);
        auto root = static_cast<InnerNode *>(index.GetRoot());

        EXPECT_EQ(root->GetCurrentSize(), 2);
        EXPECT_EQ(root->GetLowKeyPair().second->GetType(), NodeType::InnerType);
        EXPECT_EQ(root->Begin()->second->GetType(), NodeType::InnerType);

        auto pivot = root->Begin();
        EXPECT_EQ(pivot->first, 15);

        auto inner = static_cast<InnerNode *>(pivot->second);
        EXPECT_EQ(inner->GetCurrentSize(), 1);
        EXPECT_EQ(inner->Begin()->first, 21);
        EXPECT_EQ(inner->Begin()->second->GetType(), NodeType::LeafType);

        auto prev_inner = static_cast<InnerNode *>(root->GetLowKeyPair().second);
        EXPECT_EQ(prev_inner->GetCurrentSize(), 1);
        EXPECT_EQ(prev_inner->Begin()->first, 9);
        EXPECT_EQ(prev_inner->Begin()->second->GetType(), NodeType::LeafType);

        // Trigger borrow from next prev_inner node
        index.Delete( 21);

        /**
         * B+Tree after insertions:
         *
         *                          +-------------+
         *                          | * | (27, *) |
         *                          +-------------+
         *                          /            \
         *    +---------------------+           +------------------------+
         *    | * | (9, *) | (15,*) |           | * | (33, * ) | (39, *) |
         *    +---------------------+           +------------------------+
         *    /        \       \                 /         |          \
         * +---+    +----+    +--------+    +-----+    +-----+    +-----+
         * |3|6|<-->|9|12|<-->|15|18|24|<-->|27|30|<-->|33|36|<-->|39|42|
         * +---+    +----+    +--------+    +-----+    +-----+    +-----+
         *
         */
        // Verify post conditions
        EXPECT_EQ(root->GetCurrentSize(), 1);
        EXPECT_EQ(root->GetLowKeyPair().second->GetType(), NodeType::InnerType);
        EXPECT_EQ(root->Begin()->second->GetType(), NodeType::InnerType);

        EXPECT_EQ(pivot->first, 27);

        EXPECT_EQ(prev_inner->GetCurrentSize(), 2);
        EXPECT_EQ(prev_inner->Begin()->first, 9);
        EXPECT_EQ(prev_inner->Begin()->second->GetType(), NodeType::LeafType);
        EXPECT_EQ(prev_inner->RBegin()->first, 15);

        EXPECT_EQ(index.FindValueOfKey(21), std::nullopt);

        std::vector remaining_keys{3, 6, 9, 12, 15, 18, 24, 27, 30, 33, 36, 39, 42};
        // Verify forward iteration
        int i = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, remaining_keys[i++]);
        }

        // Verify backward iteration
        int j = remaining_keys.size() - 1;
        for (auto iter = index.RBegin(); iter != index.REnd(); --iter) {
            EXPECT_EQ((*iter).first, remaining_keys[j--]);
        }
    }

    TEST(BPlusTreeDeleteTest, ReplaceRootNode) {
        auto index = bplustree::BPlusTree(3, 3);

        std::vector insert_keys{3, 6, 9, 12};
        for (auto &x: insert_keys) {
            index.InsertOptimistic(std::make_pair(x, x));
        }

        /**
         *
         *          +-----------+
         *          | * | 9 | * |
         *          +-----------+
         *          /         \
         *         /           \
         *      +-------+   +--------+
         *      | 3 | 6 |   | 9 | 12 |
         *      +-------+   +--------+
         */

        // Preconditions
        EXPECT_EQ(index.GetRoot()->GetType(), NodeType::InnerType);
        auto root = static_cast<InnerNode *>(index.GetRoot());

        EXPECT_EQ(root->GetCurrentSize(), 1);
        EXPECT_EQ(root->Begin()->first, 9);

        // Collapse a level
        index.Delete( 9);
        EXPECT_EQ(index.FindValueOfKey(9), std::nullopt);

        /**
         *      +------------+
         *      | 3 | 6 | 12 | <-- New root
         *      +------------+
         */

        // Post-conditions
        EXPECT_EQ(index.GetRoot()->GetType(), NodeType::LeafType);
        auto new_root = static_cast<LeafNode *>(index.GetRoot());

        EXPECT_EQ(new_root->GetCurrentSize(), 3);
        std::vector remaining_keys{3, 6, 12};
        int i = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, remaining_keys[i++]);
        }
    }
}
