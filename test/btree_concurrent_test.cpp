#include <gtest/gtest.h>
#include <numeric>
#include <thread>
#include <random>
#include "../src/bplustree.h"

namespace bplustree {
    TEST(BPlusTreeConcurrentTest, ConcurrentInserts) {
        BPlusTree index{3, 4};

        std::vector<int> keys(1000 * 1000);
        std::iota(keys.begin(), keys.end(), 0);

        int worker_threads = 8;
        int keys_per_worker = keys.size() / worker_threads;

        auto index_insert_workload = [&](uint32_t worker_id) {
            int start = keys_per_worker * worker_id;
            int end = keys_per_worker * (worker_id + 1);

            for (int i = start; i < end; ++i) {
                index.Insert(std::make_pair(i, i));
            }
        };

        std::vector<std::thread> workers;
        for (uint32_t worker_id = 0; worker_id < worker_threads; ++worker_id) {
            workers.push_back(std::thread(index_insert_workload, worker_id));
        }

        for (uint32_t worker_id = 0; worker_id < worker_threads; ++worker_id) {
            workers[worker_id].join();
        }

        int i = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, i++);
        }
        EXPECT_EQ(i, keys.size());
    }

    TEST(BPlusTreeConcurrentTest, ConcurrentDeletes) {
        BPlusTree index{3, 4};

        std::vector<int> keys(1000 * 1000);
        std::iota(keys.begin(), keys.end(), 0);

        for (auto &key: keys) {
            index.Insert(std::make_pair(key, key));
        }

        int worker_threads = 32;
        int keys_per_worker = keys.size() / worker_threads;

        auto index_delete_workload = [&](uint32_t worker_id) {
            int start = keys_per_worker * worker_id;
            int end = keys_per_worker * (worker_id + 1);

            for (int i = start; i < end; ++i) {
                index.Delete(i);
                EXPECT_EQ(index.MaybeGet(i), std::nullopt);
            }
        };

        std::vector<std::thread> workers;
        for (uint32_t worker_id = 0; worker_id < worker_threads; ++worker_id) {
            workers.push_back(std::thread(index_delete_workload, worker_id));
        }

        for (uint32_t worker_id = 0; worker_id < worker_threads; ++worker_id) {
            workers[worker_id].join();
        }

        // Empty B+Tree
        EXPECT_EQ(index.Begin(), index.End());
        EXPECT_EQ(index.RBegin(), index.REnd());

        // Ensure insert works
        for (int i = 0; i < 1000; ++i) {
            index.Insert(std::make_pair(i, i));
            EXPECT_EQ(index.MaybeGet(i), i);
        }

        int j = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, j++);
        }
        EXPECT_EQ(j, 1000);

        int k = 999;
        for (auto iter = index.RBegin(); iter != index.REnd(); --iter) {
            EXPECT_EQ((*iter).first, k--);
        }
        EXPECT_EQ(k, -1);
    }

    TEST(BPlusTreeConcurrentTest, ConcurrentRandomInserts) {
        BPlusTree index{3, 4};

        std::vector<int> keys(1000 * 1000);
        std::iota(keys.begin(), keys.end(), 0);

        std::random_device rd{};
        std::shuffle(keys.begin(), keys.end(), std::mt19937{rd()});

        int worker_threads = 8;
        int keys_per_worker = keys.size() / worker_threads;

        auto index_insert_workload = [&](uint32_t worker_id) {
            int start = keys_per_worker * worker_id;
            int end = keys_per_worker * (worker_id + 1);

            for (int i = start; i < end; ++i) {
                index.Insert(std::make_pair(i, i));
            }
        };

        std::vector<std::thread> workers;
        for (uint32_t worker_id = 0; worker_id < worker_threads; ++worker_id) {
            workers.push_back(std::thread(index_insert_workload, worker_id));
        }

        for (uint32_t worker_id = 0; worker_id < worker_threads; ++worker_id) {
            workers[worker_id].join();
        }

        int i = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, i++);
        }
        EXPECT_EQ(i, keys.size());
    }

    TEST(BPlusTreeConcurrentTest, ConcurrentRandomDeletes) {
        BPlusTree index{3, 4};

        std::vector<int> keys(1000 * 1000);
        std::iota(keys.begin(), keys.end(), 0);

        std::random_device rd{};
        std::shuffle(keys.begin(), keys.end(), std::mt19937{rd()});

        for (auto &key: keys) {
            index.Insert(std::make_pair(key, key));
        }

        int worker_threads = 32;
        int keys_per_worker = keys.size() / worker_threads;

        auto index_delete_workload = [&](uint32_t worker_id) {
            int start = keys_per_worker * worker_id;
            int end = keys_per_worker * (worker_id + 1);

            for (int i = start; i < end; ++i) {
                index.Delete(i);
                EXPECT_EQ(index.MaybeGet(i), std::nullopt);
            }
        };

        std::vector<std::thread> workers;
        for (uint32_t worker_id = 0; worker_id < worker_threads; ++worker_id) {
            workers.push_back(std::thread(index_delete_workload, worker_id));
        }

        for (uint32_t worker_id = 0; worker_id < worker_threads; ++worker_id) {
            workers[worker_id].join();
        }

        // Empty B+Tree
        EXPECT_EQ(index.Begin(), index.End());
        EXPECT_EQ(index.RBegin(), index.REnd());
        EXPECT_EQ(index.ToGraph(), "digraph empty_bplus_tree {}");

        // Ensure insert works
        for (int i = 0; i < 1000; ++i) {
            index.Insert(std::make_pair(i, i));
            EXPECT_EQ(index.MaybeGet(i), i);
        }

        int j = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, j++);
        }
        EXPECT_EQ(j, 1000);

        int k = 999;
        for (auto iter = index.RBegin(); iter != index.REnd(); --iter) {
            EXPECT_EQ((*iter).first, k--);
        }
        EXPECT_EQ(k, -1);
    }

    TEST(BPlusTreeConcurrentTest, ConcurrentIterators) {
        BPlusTree index{3, 4};

        auto key_count = 1000 * 1000;
        std::vector<int> keys(key_count);
        std::iota(keys.begin(), keys.end(), 0);

        std::random_device rd{};
        std::shuffle(keys.begin(), keys.end(), std::mt19937{rd()});

        for (auto &key: keys) {
            index.Insert(std::make_pair(key, key));
        }

        int worker_threads = 8;
        auto iterator_workload = [&](uint32_t worker_id) {
            auto i = 0;
            auto it = index.Begin();
            for (; it != index.End() && it != index.Retry(); ++it, ++i) {
                EXPECT_EQ((*it).first, i);
                EXPECT_EQ((*it).second, i);
            }
            EXPECT_EQ(i, key_count);

            auto j = key_count - 1;
            auto rit = index.RBegin();
            for (; rit != index.REnd() && rit != index.Retry(); --rit, --j) {
                EXPECT_EQ((*rit).first, j);
                EXPECT_EQ((*rit).second, j);
            }
            EXPECT_EQ(j, -1);
        };


        std::vector<std::thread> workers;
        for (uint32_t worker_id = 0; worker_id < worker_threads; ++worker_id) {
            workers.push_back(std::thread(iterator_workload, worker_id));
        }

        for (uint32_t worker_id = 0; worker_id < worker_threads; ++worker_id) {
            workers[worker_id].join();
        }

        // ensure latches are released by attempting to insert
        auto extra_key_count = 100;
        std::vector<int> extra_keys(extra_key_count);
        std::iota(extra_keys.begin(), extra_keys.end(), key_count);

        std::shuffle(extra_keys.begin(), extra_keys.end(), std::mt19937{rd()});
        for (auto &key: extra_keys) {
            index.Insert(std::make_pair(key, key));
        }

        // WHY ARE THESE SCOPES NECESSARY? A NOTE ON CONCURRENCY.
        //
        // This B+Tree prevents deadlocks by enforcing a strict latching hierarchy:
        // any thread, for any operation, must acquire a latch on an ancestor node
        // before acquiring a latch on a descendant node (i.e., top-down traversal).
        //
        // This rule is particularly important for write operations (Insert/Delete),
        // which may take exclusive latches on nodes as they traverse the tree.
        //
        // An iterator holds a SHARED latch on the leaf it points to. If that iterator
        // remains alive while another operation attempts a new top-down traversal
        // from the root, we can get a deadlock. Consider this scenario:
        //
        // 1. Thread A: Creates an iterator, holding a SHARED latch on a leaf node.
        // 2. Thread B: Starts an INSERT, taking an EXCLUSIVE latch on the root.
        // 3. Thread B: Traverses down and tries to take an EXCLUSIVE latch on the same
        //              leaf node. It BLOCKS, waiting for Thread A's shared latch.
        // 4. Thread A: Now attempts to create a *second* iterator (e.g., by calling
        //              Begin()). It tries to take a SHARED latch on the root. It BLOCKS,
        //              waiting for Thread B's exclusive latch.
        //
        // This is a classic deadlock. The scopes below prevent it by ensuring the
        // first iterator's latch is released before the second operation begins.
        // This rule applies even if both operations are in the same thread, because
        // the latching protocol must be honored globally to ensure safety.
        {
            auto rit = index.RBegin();
            EXPECT_EQ((*rit).first, key_count + extra_key_count - 1);
        }

        {
            auto it = index.Begin();
            EXPECT_EQ((*it).first, 0);
        }
    }

    TEST(BPlusTreeConcurrentTest, HighBranchingFactorInserts) {
        BPlusTree index{63, 64};

        std::vector<int> keys(1000 * 1000);
        std::iota(keys.begin(), keys.end(), 0);

        std::random_device rd{};
        std::shuffle(keys.begin(), keys.end(), std::mt19937{rd()});

        int worker_threads = 8;
        int keys_per_worker = keys.size() / worker_threads;

        auto index_insert_workload = [&](uint32_t worker_id) {
            int start = keys_per_worker * worker_id;
            int end = keys_per_worker * (worker_id + 1);

            for (int i = start; i < end; ++i) {
                index.Insert(std::make_pair(i, i));
            }
        };

        std::vector<std::thread> workers;
        for (uint32_t worker_id = 0; worker_id < worker_threads; ++worker_id) {
            workers.push_back(std::thread(index_insert_workload, worker_id));
        }

        for (uint32_t worker_id = 0; worker_id < worker_threads; ++worker_id) {
            workers[worker_id].join();
        }

        int i = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, i++);
        }
        EXPECT_EQ(i, keys.size());
    }

    TEST(BPlusTreeConcurrentTest, HighBranchingFactorDeletes) {
        BPlusTree index{63, 64};

        std::vector<int> keys(1000 * 1000);
        std::iota(keys.begin(), keys.end(), 0);

        std::random_device rd{};
        std::shuffle(keys.begin(), keys.end(), std::mt19937{rd()});

        for (auto &key: keys) {
            index.Insert(std::make_pair(key, key));
        }

        int worker_threads = 32;
        int keys_per_worker = keys.size() / worker_threads;

        auto index_delete_workload = [&](uint32_t worker_id) {
            int start = keys_per_worker * worker_id;
            int end = keys_per_worker * (worker_id + 1);

            for (int i = start; i < end; ++i) {
                index.Delete(i);
                EXPECT_EQ(index.MaybeGet(i), std::nullopt);
            }
        };

        std::vector<std::thread> workers;
        for (uint32_t worker_id = 0; worker_id < worker_threads; ++worker_id) {
            workers.push_back(std::thread(index_delete_workload, worker_id));
        }

        for (uint32_t worker_id = 0; worker_id < worker_threads; ++worker_id) {
            workers[worker_id].join();
        }

        // Empty B+Tree
        EXPECT_EQ(index.Begin(), index.End());
        EXPECT_EQ(index.RBegin(), index.REnd());
        EXPECT_EQ(index.ToGraph(), "digraph empty_bplus_tree {}");

        // Ensure insert works
        for (int i = 0; i < 1000; ++i) {
            index.Insert(std::make_pair(i, i));
            EXPECT_EQ(index.MaybeGet(i), i);
        }

        int j = 0;
        for (auto iter = index.Begin(); iter != index.End(); ++iter) {
            EXPECT_EQ((*iter).first, j++);
        }
        EXPECT_EQ(j, 1000);

        int k = 999;
        for (auto iter = index.RBegin(); iter != index.REnd(); --iter) {
            EXPECT_EQ((*iter).first, k--);
        }
        EXPECT_EQ(k, -1);
    }


    TEST(BPlusTreeConcurrentTest, MixedWorkload) {
        BPlusTree index{3, 4};
        auto insert_workload = [&](std::vector<int> keys) {
            for (auto &key: keys) {
                index.Insert(std::make_pair(key, key));
            }
        };

        auto delete_workload = [&](std::vector<int> keys) {
            for (auto &key: keys) {
                index.Delete(key);
            }
        };

        std::random_device rd{};
        auto key_count = 1000;

        std::vector<int> even_keys(key_count / 2);
        auto e = -2;
        std::generate(even_keys.begin(), even_keys.end(), [&e] { return (e += 2); });
        std::shuffle(even_keys.begin(), even_keys.end(), std::mt19937{rd()});

        std::vector<int> odd_keys(key_count / 2);
        auto o = -1;
        std::generate(odd_keys.begin(), odd_keys.end(), [&o] { return (o += 2); });
        std::shuffle(odd_keys.begin(), odd_keys.end(), std::mt19937{rd()});

        // Construct the initial tree
        for (auto &key: odd_keys) {
            index.Insert(std::make_pair(key, key));
        }

        auto inserter = std::thread(insert_workload, even_keys);
        auto deleter = std::thread(delete_workload, odd_keys);

        inserter.join();
        deleter.join();

        // Inserted odd keys to construct initial tree
        // Inserted even keys, concurrently deleting odd keys
        EXPECT_EQ((*index.Begin()).first, 0);
        EXPECT_EQ((*index.RBegin()).first, 998);
    }
}