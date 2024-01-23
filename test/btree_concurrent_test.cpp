#include <gtest/gtest.h>
#include <numeric>
#include <thread>
#include "../src/bplustree.h"

namespace bplustree {
    TEST(BPlusTreeConcurrentTest, ConcurrentInserts) {
        BPlusTree index{3, 4};

        std::vector<int> keys(12);
        std::iota(keys.begin(), keys.end(), 0);

        int worker_threads = 2;
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
}
