#include <shared_mutex>
#include <cassert>
#include <cstdio>
#include <atomic>

/**
 * A wrapper around std::shared_mutex
 */
namespace bplustree {

    class SharedLatch {
    public:
        SharedLatch() = default;

#ifdef ENABLE_LATCH_DEBUGGING
        ~SharedLatch() {
            if (exclusive_lock_count_ != 0) {
                fprintf(stderr, "exclusive_lock_count_ is %d, expected 0 for latch %p\n", exclusive_lock_count_.load(), this);
            }
            assert(exclusive_lock_count_ == 0);
            if (shared_lock_count_ != 0) {
                fprintf(stderr, "shared_lock_count_ is %d, expected 0 for latch %p\n", shared_lock_count_.load(), this);
            }
            assert(shared_lock_count_ == 0);
        }
#endif


        void LockExclusive() {
            latch_.lock();
#ifdef ENABLE_LATCH_DEBUGGING
            exclusive_lock_count_++;
#endif
        }

        void LockShared() {
            latch_.lock_shared();
#ifdef ENABLE_LATCH_DEBUGGING
            shared_lock_count_++;
#endif
        }

        void UnlockExclusive() {
#ifdef ENABLE_LATCH_DEBUGGING
            exclusive_lock_count_--;
#endif
            latch_.unlock();
        }

        void UnlockShared() {
#ifdef ENABLE_LATCH_DEBUGGING
            shared_lock_count_--;
#endif
            latch_.unlock_shared();
        }

        bool TryLockShared() {
            bool success = latch_.try_lock_shared();
#ifdef ENABLE_LATCH_DEBUGGING
            if (success) {
                shared_lock_count_++;
            }
#endif
            return success;
        }

    private:
        std::shared_mutex latch_;
#ifdef ENABLE_LATCH_DEBUGGING
        std::atomic<int> exclusive_lock_count_{0};
        std::atomic<int> shared_lock_count_{0};
#endif
    };

}