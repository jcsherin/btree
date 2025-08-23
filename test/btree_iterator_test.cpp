/*
 * ###################################################################
 * # The Importance of Iterator Lifetime and Latching Protocol
 * ###################################################################
 *
 * The tests in this file, while seemingly simple serial tests, are critical
 * for verifying the correct usage of the B+Tree's iterator concurrency model.
 * They prevent regressions of a specific, subtle class of deadlocks that can
 * occur due to lock-order-inversion, which would be detected by tools like
 * ThreadSanitizer (TSAN).
 *
 * ## The Latching Protocol
 *
 * The B+Tree uses a strict, top-down (ancestor-before-descendant) latching
 * protocol to allow multiple readers and writers to operate concurrently
 * without deadlocking. This rule is global and must be honored by all threads.
 *
 * ## The Iterator Problem
 *
 * An iterator holds a SHARED latch on the leaf node it currently points to.
 * This is essential for its stability. However, if an iterator object remains
 * alive, its thread is holding a low-level (leaf) latch.
 *
 * If the same thread then attempts to start *any* new operation that traverses
 * from the root (e.g., Begin(), RBegin(), Insert()), that new operation must
 * first acquire a latch on the root node. This creates a lock-order-inversion:
 * the thread holds a descendant's latch and requests an ancestor's latch.
 *
 * This inversion can lead to a deadlock with a concurrent writer thread, as
 * detailed in the comments of the `BPlusTreeConcurrentTest.ConcurrentIterators` test.
 *
 * ## The Solution and Purpose of These Tests
 *
 * The only safe way to use multiple iterators or mix iterators with other
 * operations in the same thread is to ensure the previous iterator's lifetime
 * has ended (releasing its latch) before the next top-down operation begins.
 * This is achieved by scoping the iterators with `{...}`.
 *
 * The tests herein validate this behavior for various tree structures (from
 * empty to multi-level) and iterator usage patterns. They ensure that the
 * fundamental contract of iterator usage is respected and documented.
 *
 */
#include <gtest/gtest.h>
#include <numeric>
#include "../src/bplustree.h"

namespace bplustree {

    TEST(BPlusTreeIteratorTest, EmptyTree) {
        BPlusTree index{3, 4};
        EXPECT_EQ(index.Begin(), index.End());
        EXPECT_EQ(index.RBegin(), index.REnd());
    }

    TEST(BPlusTreeIteratorTest, RootOnlyTree) {
        BPlusTree index{3, 4};
        const int key_count = 3;

        for (int i = 0; i < key_count; ++i) {
            index.Insert(std::make_pair(i, i));
        }

        {
            auto it = index.Begin();
            EXPECT_NE(it, index.End());
            EXPECT_EQ((*it).first, 0);
        }

        {
            auto rit = index.RBegin();
            EXPECT_NE(rit, index.REnd());
            EXPECT_EQ((*rit).first, key_count - 1);
        }

        int i = 0;
        for (auto it = index.Begin(); it != index.End(); ++it, ++i) {
            EXPECT_EQ((*it).first, i);
        }
        EXPECT_EQ(i, key_count);

        int j = key_count - 1;
        for (auto rit = index.RBegin(); rit != index.REnd(); --rit, --j) {
            EXPECT_EQ((*rit).first, j);
        }
        EXPECT_EQ(j, -1);
    }

    TEST(BPlusTreeIteratorTest, TwoLevelTree) {
        BPlusTree index{3, 4};
        const int key_count = 5;

        for (int i = 0; i < key_count; ++i) {
            index.Insert(std::make_pair(i, i));
        }

        {
            auto it = index.Begin();
            EXPECT_NE(it, index.End());
            EXPECT_EQ((*it).first, 0);
        }

        {
            auto rit = index.RBegin();
            EXPECT_NE(rit, index.REnd());
            EXPECT_EQ((*rit).first, key_count - 1);
        }

        int i = 0;
        for (auto it = index.Begin(); it != index.End(); ++it, ++i) {
            EXPECT_EQ((*it).first, i);
        }
        EXPECT_EQ(i, key_count);

        int j = key_count - 1;
        for (auto rit = index.RBegin(); rit != index.REnd(); --rit, --j) {
            EXPECT_EQ((*rit).first, j);
        }
        EXPECT_EQ(j, -1);
    }

    TEST(BPlusTreeIteratorTest, ThreeLevelTree) {
        BPlusTree index{3, 4};
        const int key_count = 10;

        for (int i = 0; i < key_count; ++i) {
            index.Insert(std::make_pair(i, i));
        }

        {
            auto it = index.Begin();
            EXPECT_NE(it, index.End());
            EXPECT_EQ((*it).first, 0);
        }

        {
            auto rit = index.RBegin();
            EXPECT_NE(rit, index.REnd());
            EXPECT_EQ((*rit).first, key_count - 1);
        }

        int i = 0;
        for (auto it = index.Begin(); it != index.End(); ++it, ++i) {
            EXPECT_EQ((*it).first, i);
        }
        EXPECT_EQ(i, key_count);

        int j = key_count - 1;
        for (auto rit = index.RBegin(); rit != index.REnd(); --rit, --j) {
            EXPECT_EQ((*rit).first, j);
        }
        EXPECT_EQ(j, -1);
    }

    TEST(BPlusTreeIteratorTest, ScopedIteratorSameTypeUsage) {
        BPlusTree index{3, 4};
        const int key_count = 10;

        for (int i = 0; i < key_count; ++i) {
            index.Insert(std::make_pair(i, i));
        }

        {
            auto it1 = index.Begin();
            EXPECT_NE(it1, index.End());
            EXPECT_EQ((*it1).first, 0);
        }

        {
            auto it2 = index.Begin();
            EXPECT_NE(it2, index.End());
            EXPECT_EQ((*it2).first, 0);
        }

        {
            auto rit1 = index.RBegin();
            EXPECT_NE(rit1, index.REnd());
            EXPECT_EQ((*rit1).first, key_count - 1);
        }

        {
            auto rit2 = index.RBegin();
            EXPECT_NE(rit2, index.REnd());
            EXPECT_EQ((*rit2).first, key_count - 1);
        }
    }
}
