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
    }
}
