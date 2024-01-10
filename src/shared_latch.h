#include <shared_mutex>

/**
 * A wrapper around std::shared_mutex
 */
namespace bplustree {

    class SharedLatch {
    public:
        void LockExclusive() { latch_.lock(); }

        void LockShared() { latch_.lock_shared(); }

        void UnlockExclusive() { latch_.unlock(); }

        void UnlockShared() { latch_.unlock_shared(); }

        bool TryLockShared() { return latch_.try_lock_shared(); }

    private:
        std::shared_mutex latch_;
    };
}

