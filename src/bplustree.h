#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <cstring>
#include <algorithm>
#include <utility>
#include <vector>
#include <sstream>
#include <queue>
#include <optional>
#include "macros.h"
#include "shared_latch.h"

namespace bplustree {
    enum class NodeType : int {
        InnerType = 0, LeafType = 1
    };

    /**
     * Helper for minimum node occupancy calculation based on node fanout
     * https://stackoverflow.com/questions/2745074/fast-ceiling-of-an-integer-division-in-c-c
     *
     * @param x
     * @param y
     * @return ceiling of x when divided by y
     */
    int FastCeilIntDivision(int x, int y) {
        BPLUSTREE_ASSERT(x != 0, "x should be greater than zero");

        return 1 + ((x - 1) / y);
    }


    class BaseNode {
    public:
        BaseNode(NodeType p_type, int p_max_size) :
                type_{p_type},
                max_size_{p_max_size} {}

        NodeType GetType() { return type_; }

        int GetMaxSize() { return max_size_; }

        void GetNodeExclusiveLatch() { node_latch_.LockExclusive(); }

        void GetNodeSharedLatch() { node_latch_.LockShared(); }

        void ReleaseNodeExclusiveLatch() { node_latch_.UnlockExclusive(); }

        void ReleaseNodeSharedLatch() { node_latch_.UnlockShared(); }

        bool TrySharedLock() { return node_latch_.TryLockShared(); }

    private:
        NodeType type_;
        int max_size_;
        SharedLatch node_latch_;
    };

    using KeyNodePointerPair = std::pair<int, BaseNode *>;
    using KeyValuePair = std::pair<int, int>;

    template<typename ElementType>
    class ElasticNode : public BaseNode {
    public:
        ElasticNode(NodeType p_type, KeyNodePointerPair p_low_key, int p_max_size) :
                BaseNode(p_type, p_max_size),
                low_key_{p_low_key},
                sibling_left_{nullptr},
                sibling_right_{nullptr},
                end_{start_} {}

        /**
         *
         * @return a pointer to the new node after splitting the current
         * node. Returns `nullptr` if the node is not already full.
         */
        ElasticNode *SplitNode() {
            if (this->GetCurrentSize() < this->GetMaxSize()) return nullptr;

            ElasticNode *new_node = this->Get(this->GetType(), this->GetLowKeyPair(), this->GetMaxSize());
            ElementType *copy_from_location = std::next(this->Begin(), FastCeilIntDivision(this->GetCurrentSize(), 2));

            std::memcpy(
                    reinterpret_cast<void *>(new_node->Begin()),
                    reinterpret_cast<void *>(copy_from_location),
                    std::distance(copy_from_location, this->End()) * sizeof(ElementType)
            );
            new_node->SetEnd(std::distance(copy_from_location, this->End()));
            end_ = copy_from_location;

            return new_node;
        }

        /**
         * Static helper to allocate storage for an elastic node.
         */
        static ElasticNode *Get(NodeType p_type, KeyNodePointerPair p_low_key, int p_max_size) {
            auto *alloc = new char[sizeof(ElasticNode) + p_max_size * sizeof(ElementType)];

            /**
             * Construct elastic node in allocated storage
             * https://en.cppreference.com/w/cpp/language/new#Placement_new
             */
            auto elastic_node = reinterpret_cast<ElasticNode *>(alloc);
            new(elastic_node) ElasticNode(p_type, p_low_key, p_max_size);

            return elastic_node;
        }

        /**
         * Free elastic node
         */
        void FreeElasticNode() {
            ElasticNode *alloc = this;
            delete[] reinterpret_cast<char *>(alloc);
        }

        void SetEnd(int offset) { end_ = start_ + offset; }

        ElementType *Begin() { return start_; }

        ElementType *RBegin() {
            if (this->GetCurrentSize() == 0) { return nullptr; }

            return std::prev(this->End());
        }

        const ElementType *Begin() const { return start_; }

        ElementType *End() { return end_; }

        const ElementType *End() const { return end_; }

        int GetCurrentSize() const {
            return std::distance(Begin(), End());
        }

        bool InsertElementIfPossible(const ElementType &element, ElementType *location) {
            if (GetCurrentSize() >= GetMaxSize()) { return false; }

            if (std::distance(location, End()) > 0) {
                std::memmove(
                        reinterpret_cast<void *>(std::next(location)),
                        reinterpret_cast<void *>(location),
                        std::distance(location, End()) * sizeof(ElementType)
                );
            }
            new(location) ElementType{element};
            std::advance(end_, 1);

            return true;
        }

        bool DeleteElement(ElementType *location) {
            if (std::distance(Begin(), location) < 0) { return false; }

            if (GetCurrentSize() == 1) {
                SetEnd(0);
                return true;
            }

            std::memmove(reinterpret_cast<void *>(location),
                         reinterpret_cast<void *>(std::next(location)),
                         (std::distance(location, End()) - 1) * sizeof(ElementType));
            SetEnd(GetCurrentSize() - 1);
            return true;
        }

        bool PopBegin() {
            if (GetCurrentSize() == 0) { return false; }
            if (GetCurrentSize() == 1) {
                SetEnd(0);
                return true;
            }

            std::memmove(reinterpret_cast<void *>(start_),
                         reinterpret_cast<void *>(start_ + 1),
                         (this->GetCurrentSize() - 1) * sizeof(ElementType));
            SetEnd(this->GetCurrentSize() - 1);
            return true;
        }

        bool PopEnd() {
            if (GetCurrentSize() == 0) { return false; }
            if (GetCurrentSize() == 1) {
                SetEnd(0);
                return true;
            }

            SetEnd(this->GetCurrentSize() - 1);
            return true;
        }

        bool MergeNode(ElasticNode *next_node) {
            if (this->GetType() != next_node->GetType()) {
                return false;
            }

            if (this->GetCurrentSize() + next_node->GetCurrentSize() > this->GetMaxSize()) {
                return false;
            }

            std::memmove(reinterpret_cast<void *>(this->End()),
                         reinterpret_cast<void *>(next_node->Begin()),
                         next_node->GetCurrentSize() * sizeof(ElementType));
            SetEnd(this->GetCurrentSize() + next_node->GetCurrentSize());
            return true;
        }

        BaseNode *GetSiblingLeft() { return sibling_left_; }

        BaseNode *GetSiblingRight() { return sibling_right_; }

        void SetSiblingLeft(BaseNode *node) { sibling_left_ = node; }

        void SetSiblingRight(BaseNode *node) { sibling_right_ = node; }

        KeyNodePointerPair &GetLowKeyPair() { return low_key_; }

        void SetLowKeyPair(const KeyNodePointerPair p_low_key) { low_key_ = p_low_key; }

        const ElementType &At(const int index) {
            return *(std::next(Begin(), index));
        }

    private:
        // 1. An inner node stores N keys, and (N+1) node pointers. This is
        // the extra node pointer in the inner node. It points to the node
        // with keys less than the smallest key in the inner node. This is
        // always set to `nullptr` in leaf nodes, and has no meaning.
        //
        // 2. The addition of this node pointer separate from the `start_`
        // internal array simplifies code which deals with searching, splitting
        // and merging of inner nodes. This is because the to store the extra
        // node pointer, we leave the first key as invalid. The actual first
        // key therefore begins at index 1 and any code which dealt with inner
        // nodes have to handle this special case. This specialization is not
        // necessary anymore with the addition of this field.
        //
        // 3. This is added here instead of in the derived `InnerNode` class
        // because it will clash with the `start_` array which is invisible
        // to the compiler.
        KeyNodePointerPair low_key_;

        // Sibling pointers for chaining leaf nodes
        BaseNode *sibling_left_{nullptr};
        BaseNode *sibling_right_{nullptr};

        // Points to location one past the last element in `start_`
        ElementType *end_;

        /*
         * Struct hack (flexible array member)
         * https://developers.redhat.com/articles/2022/09/29/benefits-limitations-flexible-array-members
         *
         * The array for holding the key-value elements
         */
        ElementType start_[0];
    };

    class InnerNode : public ElasticNode<KeyNodePointerPair> {
    public:
        /**
         * Use the `ElasticNode` interface for constructing an `InnerNode`
         */
        InnerNode() = delete;

        InnerNode(const InnerNode &) = delete;

        InnerNode &operator=(const InnerNode &) = delete;

        InnerNode(InnerNode &&) = delete;

        InnerNode &operator=(InnerNode &&) = delete;

        ~InnerNode() {
            this->~ElasticNode<KeyNodePointerPair>();
        }

        KeyNodePointerPair *FindLocation(const int key) {
            BaseNode *dummy_ptr = nullptr;
            KeyNodePointerPair dummy_element = std::make_pair(key, dummy_ptr);

            KeyNodePointerPair *iter = std::lower_bound(
                    this->Begin(), this->End(), dummy_element,
                    [](const auto &a, const auto &b) { return a < b; }
            );

            return iter;
        }

        /**
         * Calculating minimum occupancy of inner node
         * -------------------------------------------
         * Fanout: N    = 4 (node pointers)
         * No. of keys  = N - 1
         *              = 4 - 1
         *              = 3
         *
         * Minimum node pointers = ⌈N/2⌉ (14.3.3.2 B+Tree Deletion)
         *                       = ⌈4/2⌉
         *                       = 2
         * Minimum no. of keys   = 2 - 1
         *                       = 1
         *
         * The smallest possible inner node has at least one key and two
         * node pointers. This is an inner node which has a fanout of
         * 3 node pointers, and can store a maximum of 2 keys.
         *
         * @return The minimum no. of keys which should be present in
         * the inner node at any point.
         */
        int GetMinSize() {
            int fanout = this->GetMaxSize() + 1;
            int minimum_fanout = FastCeilIntDivision(fanout, 2);

            return (minimum_fanout - 1);
        }

        /**
         *
         * @param search_key The key for lower bound search
         * @return The location of the pivot element in the inner node
         * which points to the child node
         */
        KeyNodePointerPair *FindPivot(int search_key) {
            auto iter = FindLocation(search_key);

            if (iter != End() && search_key == iter->first) {
                return iter;
            }

            if (iter == Begin() && search_key < iter->first) {
                return &GetLowKeyPair();
            }

            return std::prev(iter);
        }

        /**
         *
         * @param search_key
         * @return An optional pair of previous node pointer, and the element
         * which separates it from the node pointer determined by the search
         * key. When the search key already points to the left most node
         * pointer, there is no previous and it returns a null optional.
         */
        std::optional<std::pair<BaseNode *, KeyNodePointerPair *>> MaybePreviousWithSeparator(int search_key) {
            auto pivot = FindPivot(search_key);

            // Left most child pointer, has no previous sibling
            if (pivot->second == GetLowKeyPair().second) {
                return {};
            }

            if (pivot == Begin()) {
                return std::make_pair(GetLowKeyPair().second, pivot);
            }

            return std::make_pair(std::prev(pivot)->second, pivot);
        }

        std::optional<std::pair<BaseNode *, KeyNodePointerPair *>> MaybeNextWithSeparator(int search_key) {
            auto pivot = FindPivot(search_key);

            // Right most element, has no next element
            if (std::next(pivot) == End()) {
                return {};
            }

            if (pivot->second == GetLowKeyPair().second) {
                return std::make_pair(Begin()->second, Begin());
            }

            return std::make_pair(std::next(pivot)->second, std::next(pivot));

        }
    };

    class BPlusTree;

    class LeafNode : public ElasticNode<KeyValuePair> {
    public:
        /**
         * Use the `ElasticNode` interface for constructing an `LeafNode`
         */
        LeafNode() = delete;

        LeafNode(const LeafNode &) = delete;

        LeafNode &operator=(const LeafNode &) = delete;

        LeafNode(LeafNode &&) = delete;

        LeafNode &operator=(LeafNode &&) = delete;

        ~LeafNode() {
            this->~ElasticNode<KeyValuePair>();
        }

        KeyValuePair *FindLocation(const int key) {
            KeyValuePair dummy = std::make_pair(key, 0);
            KeyValuePair *iter = std::lower_bound(this->Begin(), this->End(), dummy,
                                                  [](const auto &a, const auto &b) {
                                                      return a < b;
                                                  });
            return iter;
        }

        /**
         * The textbook defines leaf node underflow when it has less than
         * ⌈(N-1)/2⌉ values. This assumes the fanout is defined as the same
         * for both inner and leaf nodes. This implementation allows different
         * values to be set for inner and leaf node fanout. Also a leaf node
         * unlike an inner node has the same no. of keys and value pairs.
         *
         * I assume the (N-1) adjustment exists because leaf nodes have equal
         * no. of keys and values. But this adjust is not necessary here
         * because the fanout for leaf node is configured separately from the
         * inner nodes when a B+Tree instance is instantiated. So to maintain
         * 50% occupancy we'll compute the minimum as ⌈N/2⌉.
         *
         * +---+-------+-----------+
         * | N | ⌈N/2⌉ | ⌈(N-1)/2⌉ |
         * +---+-------+-----------+
         * | 3 |    2  |        1  |
         * +---+-------+-----------+
         * | 4 |    2  |        2  |
         * +---+-------+-----------+
         *
         * The implementation choice is ⌈N/2⌉ because it guarantees at least
         * two values will be present in a leaf node even when the fanout is
         * as low as 3. In practice you'll not create B+Tree nodes which such
         * low fanout, but this is here so that we can easily test the B+Tree
         * behaviour using low fanout numbers.
         *
         * @return The minimum no. of key-value pairs which should be present
         * in the leaf node
         */
        int GetMinSize() {
            return FastCeilIntDivision(this->GetMaxSize(), 2);
        }
    };

    class BPlusTreeIterator {
    public:
        BPlusTreeIterator(ElasticNode<KeyValuePair> *node, KeyValuePair *element) :
                current_node_{node},
                current_element_{element},
                state_{IteratorState::VALID} {}

        BPlusTreeIterator() :
                current_node_{nullptr},
                current_element_{nullptr},
                state_{IteratorState::INVALID} {}

        ~BPlusTreeIterator() {
            current_node_ = nullptr;
            current_element_ = nullptr;
            state_ = INVALID;
        }

        BPlusTreeIterator(const BPlusTreeIterator &) = delete;

        BPlusTreeIterator &operator=(const BPlusTreeIterator &) = delete;

        BPlusTreeIterator(BPlusTreeIterator &&other) noexcept {
            current_node_ = other.current_node_;
            current_element_ = other.current_element_;
            state_ = other.state_;

            other.current_node_ = nullptr;
            other.current_element_ = nullptr;
            state_ = IteratorState::INVALID;
        }

        BPlusTreeIterator &operator=(BPlusTreeIterator &&other) noexcept {
            if (this != &other) {
                current_node_ = other.current_node_;
                current_element_ = other.current_element_;
                state_ = other.state_;

                other.current_node_ = nullptr;
                other.current_element_ = nullptr;
                state_ = IteratorState::INVALID;
            }

            return *this;
        }

        auto operator*() -> KeyValuePair & {
            return *current_element_;
        }

        void operator++() {
            BPLUSTREE_ASSERT(state_ == VALID, "Iterator in invalid state.");

            if (std::next(current_element_) != current_node_->End()) {
                current_element_ = std::next(current_element_);
                return;
            }

            if (current_node_->GetSiblingRight() == nullptr) {
                current_node_->ReleaseNodeSharedLatch();
                SetEndIterator();
                return;
            }

            auto previous_node = current_node_;
            current_node_ = static_cast<ElasticNode<KeyValuePair> *>(current_node_->GetSiblingRight());

            if (!(current_node_->TrySharedLock())) {
                previous_node->ReleaseNodeSharedLatch();
                SetRetryIterator();
                return;
            }
            previous_node->ReleaseNodeSharedLatch();

            current_element_ = current_node_->Begin();
        }

        void operator--() {
            BPLUSTREE_ASSERT(state_ == VALID, "Iterator in invalid state");

            if (current_element_ != current_node_->Begin()) {
                current_element_ = std::prev(current_element_);
                return;
            }

            if (current_node_->GetSiblingLeft() == nullptr) {
                current_node_->ReleaseNodeSharedLatch();
                SetREndIterator();
                return;
            }

            auto previous_node = current_node_;
            current_node_ = static_cast<ElasticNode<KeyValuePair> *>(current_node_->GetSiblingLeft());

            if (!(current_node_->TrySharedLock())) {
                previous_node->ReleaseNodeSharedLatch();
                SetRetryIterator();
                return;
            }
            previous_node->ReleaseNodeSharedLatch();

            current_element_ = std::prev(current_node_->End());
        }

        bool operator==(const BPlusTreeIterator &other) const {
            return (current_element_ == other.current_element_
                    && current_node_ == other.current_node_
                    && state_ == other.state_);
        }

        bool operator!=(const BPlusTreeIterator &other) const {
            return !(current_element_ == other.current_element_
                     && current_node_ == other.current_node_
                     && state_ == other.state_);
        }

        static BPlusTreeIterator GetEndIterator() {
            auto iter = BPlusTreeIterator();
            iter.SetEndIterator();
            return iter;
        }

        static BPlusTreeIterator GetREndIterator() {
            auto iter = BPlusTreeIterator();
            iter.SetREndIterator();
            return iter;
        }

        static BPlusTreeIterator GetRetryIterator() {
            auto iter = BPlusTreeIterator();
            iter.SetRetryIterator();
            return iter;
        }

    private:
        enum IteratorState {
            VALID, INVALID, END, REND, RETRY
        };

        // Iterator is currently at this leaf node
        ElasticNode<KeyValuePair> *current_node_;

        // Pointer to element in current leaf node
        KeyValuePair *current_element_;

        IteratorState state_;

        void ResetIterator() {
            current_node_ = nullptr;
            current_element_ = nullptr;
        }

        void SetEndIterator() {
            ResetIterator();
            state_ = END;
        };

        void SetREndIterator() {
            ResetIterator();
            state_ = REND;
        }

        void SetRetryIterator() {
            ResetIterator();
            state_ = RETRY;
        }
    };

    class BPlusTree {
    public:
        explicit BPlusTree(int p_inner_node_max_size, int p_leaf_node_max_size) :
                root_{nullptr},
                inner_node_max_size_{p_inner_node_max_size},
                leaf_node_max_size_{p_leaf_node_max_size} {}

        ~BPlusTree() { FreeTree(); }

        // Used only for testing
        BaseNode *GetRoot() { return root_; }

        BPlusTreeIterator End() {
            return BPlusTreeIterator::GetEndIterator();
        }

        BPlusTreeIterator REnd() {
            return BPlusTreeIterator::GetREndIterator();
        }

        BPlusTreeIterator Retry() {
            return BPlusTreeIterator::GetRetryIterator();
        }

        BPlusTreeIterator Begin() {
            auto current_node = FindLeafNode();
            if (current_node == nullptr) {
                return End();
            }

            auto node = reinterpret_cast<ElasticNode<KeyValuePair> *>(current_node);
            return BPlusTreeIterator(node, node->Begin());
        }

        BPlusTreeIterator RBegin() {
            auto current_node = FindLastLeafNode();
            if (current_node == nullptr) {
                return REnd();
            }

            auto node = reinterpret_cast<ElasticNode<KeyValuePair> *>(current_node);
            return BPlusTreeIterator(node, node->RBegin());
        }

        BaseNode *FindLeafNode() {
            root_latch_.LockShared();
            if (root_ == nullptr) {
                root_latch_.UnlockShared();
                return nullptr;
            }

            BaseNode *current = root_;
            BaseNode *parent = nullptr;

            current->GetNodeSharedLatch();
            root_latch_.UnlockShared();

            while (current->GetType() != NodeType::LeafType) {
                parent = current;
                current = static_cast<InnerNode *>(current)->GetLowKeyPair().second;

                current->GetNodeSharedLatch();
                parent->ReleaseNodeSharedLatch();
            }

            return current;
        }

        BaseNode *FindLastLeafNode() {
            root_latch_.LockShared();
            if (root_ == nullptr) {
                root_latch_.UnlockShared();
                return nullptr;
            }

            BaseNode *current = root_;
            BaseNode *parent = nullptr;

            current->GetNodeSharedLatch();
            root_latch_.UnlockShared();

            while (current->GetType() != NodeType::LeafType) {
                parent = current;
                current = std::prev(static_cast<InnerNode *>(current)->End())->second;

                current->GetNodeSharedLatch();
                parent->ReleaseNodeSharedLatch();
            }

            return current;
        }

        std::optional<int> MaybeGet(int key) {
            root_latch_.LockShared();

            if (root_ == nullptr) {
                root_latch_.UnlockShared();
                return std::nullopt;
            }

            BaseNode *current_node = root_;
            BaseNode *parent_node = nullptr;

            current_node->GetNodeSharedLatch();
            root_latch_.UnlockShared();

            while (current_node->GetType() != NodeType::LeafType) {
                parent_node = current_node;
                current_node = static_cast<InnerNode *>(current_node)->FindPivot(key)->second;

                current_node->GetNodeSharedLatch();
                parent_node->ReleaseNodeSharedLatch();
            }

            // Beyond this point shared latch is only held on the leaf node
            auto node = reinterpret_cast<ElasticNode<KeyValuePair> *>(current_node);
            auto iter = static_cast<LeafNode *>(node)->FindLocation(key);

            auto result = (iter == node->End() | key != iter->first) ? std::nullopt : std::optional<int>{iter->second};
            current_node->ReleaseNodeSharedLatch();

            return result;
        }

        void FreeTree() {
            if (root_ == nullptr) { return; }

            std::queue<BaseNode *> collect_queue;
            std::queue<BaseNode *> free_queue;
            collect_queue.push(root_);

            while (!collect_queue.empty()) {
                auto current_node = collect_queue.front();
                collect_queue.pop();
                free_queue.push(current_node);

                if (current_node->GetType() != NodeType::LeafType) {
                    auto node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(current_node);

                    collect_queue.push(node->GetLowKeyPair().second);
                    auto current_element = node->Begin();
                    while (current_element != node->End()) {
                        collect_queue.push(current_element->second);
                        current_element = std::next(current_element);
                    }
                }
            }

            while (!free_queue.empty()) {
                auto current_node = free_queue.front();
                free_queue.pop();

                if (current_node->GetType() != NodeType::LeafType) {
                    auto node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(current_node);
                    node->FreeElasticNode();
                } else {
                    auto node = reinterpret_cast<ElasticNode<KeyValuePair> *>(current_node);
                    node->FreeElasticNode();
                }
            }
        }

        bool ReleaseAllWriteLatches(std::vector<BaseNode *> &latches, bool holds_root_latch) {
            while (!latches.empty()) {
                (*latches.rbegin())->ReleaseNodeExclusiveLatch();
                latches.pop_back();
            }

            if (holds_root_latch) {
                root_latch_.UnlockExclusive();
                holds_root_latch = false;
            }

            return holds_root_latch;
        }

        /**
         * Concurrency:
         *
         * In the first attempt we descend the B+Tree from the root to the
         * leaf node assuming that this insertion will not overflow the leaf
         * node. So an exclusive lock is only acquired on the leaf node where
         * the key-value element is written. For all parent nodes on that path
         * to the leaf node we acquire only shared locks. In latch crabbing
         * the shared latch on the parent node is released once we have
         * acquired a shared/exclusive latch on the current node.
         *
         * The root node is protected by a second latch in the B+Tree. This
         * is necessary as the root of the B+Tree itself could change while
         * the insert is in progress. We always acquire an exclusive latch
         * for the root node to prevent concurrent modifications to the
         * root of the B+Tree.
         *
         * Deadlocks are avoided by always acquiring the latches only in
         * a single direction - from the root the leaf nodes of the B+Tree.
         *
         * When the leaf node will overflow, the optimistic approach has
         * to be abandoned. The traversal process is restarted. This time
         * exclusive latches have to be acquired all the way from the
         * root down to the leaf node.
         *
         * Holding exclusive latches all the way from the root to the leaf
         * node can significantly impact concurrency. To improve upon this
         * we check if there is a chance the current Inner node will not
         * overflow if one more element is inserted. If the Inner node will
         * not overflow it is considered safe. For a safe Inner node we
         * release all the parent locks. This way we hold only just enough
         * exclusive latches to ensure write happens correctly.
         *
         * It is likely that sometimes an insert will propagate all the
         * way to the root node, changing the root of the B+Tree itself.
         * This extreme case is likely to happen less often. Most often
         * the inserts will happen in the optimistic path.
         *
         * @param key
         * @param value
         * @return true when insert is successful, false otherwise
         *
         * No support for duplicate keys. When a key already exists in the
         * tree the insertion will not overwrite the existing key, and will
         * return false.
         */
        bool Insert(const KeyValuePair element) {
            root_latch_.LockExclusive();

            if (root_ == nullptr) {
                // Create an empty leaf node, which is also the root
                KeyNodePointerPair dummy_low_key = std::make_pair(element.first, nullptr);
                root_ = ElasticNode<KeyValuePair>::Get(NodeType::LeafType, dummy_low_key, leaf_node_max_size_);
            }

            BaseNode *current_node = root_;
            BaseNode *parent_node = nullptr;

            current_node->GetNodeSharedLatch();

            // Search for leaf node traversing down the B+Tree
            while (current_node->GetType() != NodeType::LeafType) {
                if (parent_node != nullptr) {
                    parent_node->ReleaseNodeSharedLatch();
                } else {
                    root_latch_.UnlockExclusive();
                }

                parent_node = current_node;
                current_node = static_cast<InnerNode *>(current_node)->FindPivot(element.first)->second;
                current_node->GetNodeSharedLatch();
            }

            current_node->ReleaseNodeSharedLatch();
            current_node->GetNodeExclusiveLatch();
            if (parent_node != nullptr) {
                parent_node->ReleaseNodeSharedLatch();
            } else {
                root_latch_.UnlockExclusive();
            }

            auto node = reinterpret_cast<ElasticNode<KeyValuePair> *>(current_node);
            auto iter = static_cast<LeafNode *>(node)->FindLocation(element.first);

            if (iter != node->End() && element.first == iter->first) { // Duplicate insertion
                node->ReleaseNodeExclusiveLatch();
                return false;
            }

            if (node->InsertElementIfPossible(element, iter)) {
                node->ReleaseNodeExclusiveLatch();
                return true;
            }

            node->ReleaseNodeExclusiveLatch();
            /*
             * Optimistic insertion failed, so now we acquire exclusive locks
             * by restarting the traversal from the root of the B+Tree. If a
             * node is safe, we release all the parent exclusive locks.
             *
             * This is the pessimistic insertion!
             */
            bool insertion_finished = false;

            root_latch_.LockExclusive();
            bool holds_root_latch = true;

            current_node = root_;
            current_node->GetNodeExclusiveLatch();

            std::vector<BaseNode *> stack_traversed_nodes{};

            while (current_node->GetType() != NodeType::LeafType) {
                auto node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(current_node);

                // Release all parent exclusive locks if this inner node is safe
                if (node->GetCurrentSize() < node->GetMaxSize()) {
                    holds_root_latch = ReleaseAllWriteLatches(stack_traversed_nodes, holds_root_latch);
                }

                stack_traversed_nodes.push_back(current_node);
                current_node = static_cast<InnerNode *>(node)->FindPivot(element.first)->second;
                current_node->GetNodeExclusiveLatch();
            }

            node = reinterpret_cast<ElasticNode<KeyValuePair> *>(current_node);

            /**
             * Duplicates work done in the optimistic approach:
             * - In between the failed optimistic run and this run it is possible
             * that a duplicate key may have been inserted.
             * - It is possible that the leaf node which was full is not full
             * anymore, and we can insert the key-value element immediately
             * without further splitting.
             */
            iter = static_cast<LeafNode *>(node)->FindLocation(element.first);

            if (iter != node->End() && element.first == iter->first) { // Duplicate insertion
                node->ReleaseNodeExclusiveLatch();
                ReleaseAllWriteLatches(stack_traversed_nodes, holds_root_latch);

                return false;
            }

            if (node->InsertElementIfPossible(element, iter)) {
                node->ReleaseNodeExclusiveLatch();
                ReleaseAllWriteLatches(stack_traversed_nodes, holds_root_latch);

                return true;
            }

            auto split_node = node->SplitNode();
            if (element.first >= split_node->Begin()->first) {
                split_node->InsertElementIfPossible(
                        element,
                        static_cast<LeafNode *>(split_node)->FindLocation(element.first)
                );
            } else {
                node->InsertElementIfPossible(
                        element,
                        static_cast<LeafNode *>(node)->FindLocation(element.first)
                );

                /**
                 * Fix underflow in split node
                 * ---------------------------
                 *
                 * When the maximum elements allowed in the leaf is of size 3, the
                 * underflow happens when a leaf node does not have at least 2
                 * elements in it. But at size 3, the original node retains 2
                 * elements and the split node will have only a single element.
                 * This split under flows as soon as it was created.
                 *
                 * If the insertion does not happen in the split node then the
                 * leaf node must not underflow invariant will be broken after
                 * insertion.
                 *
                 * We can borrow the last element in the original node after
                 * the new element is inserted in to the correct order in it.
                 * So now both leaf nodes will have exactly 2 elements after
                 * insertion and the minimum size invariants will hold.
                 */

                if (split_node->GetCurrentSize() < static_cast<LeafNode *>(split_node)->GetMinSize()) {
                    split_node->InsertElementIfPossible(
                            (*node->RBegin()),
                            static_cast<LeafNode *>(split_node)->FindLocation(node->RBegin()->first)
                    );
                    node->PopEnd();
                }
            }

            if (node->GetSiblingRight() != nullptr) {
                auto sibling_right = reinterpret_cast<ElasticNode<KeyValuePair> *>(node->GetSiblingRight());

                sibling_right->GetNodeExclusiveLatch();
                sibling_right->SetSiblingLeft(split_node);
                sibling_right->ReleaseNodeExclusiveLatch();
            }
            // Fix sibling pointers to maintain a bidirectional chain
            // of leaf nodes
            split_node->SetSiblingLeft(node);
            split_node->SetSiblingRight(node->GetSiblingRight());
            node->SetSiblingRight(split_node);

            /**
             * Bug:
             *
             * This one was a difficult one to track down. When the optimistic
             * approach failed because the node will split, the exclusive lock
             * acquired on the leaf node which split was not released. This
             * was caught by testing insertions in random order.
             *
             * Since the exclusive write latch held on the leaf node was never
             * relinquished any subsequent insert which arrived at this leaf
             * node will end up deadlocking as it could not acquire an
             * exclusive write latch anymore.
             */
            node->ReleaseNodeExclusiveLatch();

            KeyNodePointerPair inner_node_element = std::make_pair(split_node->Begin()->first, split_node);
            while (!insertion_finished && !stack_traversed_nodes.empty()) {
                auto inner_node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(*stack_traversed_nodes.rbegin());
                stack_traversed_nodes.pop_back();

                if (inner_node->InsertElementIfPossible(
                        inner_node_element,
                        static_cast<InnerNode *>(inner_node)->FindLocation(inner_node_element.first)
                )) {
                    // we are done if insertion is possible without further splitting
                    insertion_finished = true;
                } else {
                    // Recursively split the node
                    auto split_inner_node = inner_node->SplitNode();

                    /**
                     * A bug narrative:
                     * When the fanout is 4, the number of keys is 3. After
                     * splitting the node it will have only a single element,
                     * meaning a single node pointer. This is not enough as
                     * the minimum necessary occupancy is at least 2 node
                     * pointers with a single key.
                     *
                     * We can borrow a node pointer from the original node
                     * after the split. The key of the borrowed element will
                     * become the pivot/transition key in the parent inner
                     * node to the newly split node.
                     *
                     * The incorrect implementation did not borrow a node
                     * pointer from the original node but used the first
                     * element in the split node. This results in an
                     * underflow when the node size is 3(= fanout 4).
                     *
                     * The below code sets the low key pair by borrowing
                     * the node pointer from the original node before
                     * attempting to reinsert the element which caused
                     * the split in the first place.
                     */
                    split_inner_node->GetLowKeyPair().first = inner_node->RBegin()->first;
                    split_inner_node->GetLowKeyPair().second = inner_node->RBegin()->second;
                    inner_node->PopEnd();

                    if (inner_node_element.first >= split_inner_node->GetLowKeyPair().first) {
                        split_inner_node->InsertElementIfPossible(inner_node_element,
                                                                  static_cast<InnerNode *>(split_inner_node)->FindLocation(
                                                                          inner_node_element.first));
                    } else {
                        inner_node->InsertElementIfPossible(inner_node_element,
                                                            static_cast<InnerNode *>(inner_node)->FindLocation(
                                                                    inner_node_element.first));
                    }

                    inner_node_element = std::make_pair(split_inner_node->GetLowKeyPair().first, split_inner_node);
                }

                inner_node->ReleaseNodeExclusiveLatch();
            }

            // If insertion has not finished_insertion by now the split propagated
            // back up to the root. So we now need to split the root node
            // as well
            if (!insertion_finished) {
                BPLUSTREE_ASSERT(holds_root_latch, "Holds exclusive lock on root of the B+Tree");
                auto old_root = root_;

                KeyNodePointerPair low_key = std::make_pair(inner_node_element.first, old_root);
                root_ = ElasticNode<KeyNodePointerPair>::Get(NodeType::InnerType, low_key, inner_node_max_size_);

                auto new_root = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(root_);
                new_root->InsertElementIfPossible(inner_node_element,
                                                  static_cast<InnerNode *>(new_root)->FindLocation(
                                                          inner_node_element.first));
            }

            if (holds_root_latch) {
                root_latch_.UnlockExclusive();
                holds_root_latch = false;
            }

            return true;
        }

        bool Delete(const int keyToRemove) {
            /**
             * Optimistic Approach:
             *
             * Assume the leaf node will not underflow when this key-value
             * element is removed. Therefore we do not need to acquire
             * exclusive write latches while traversing the B+Tree. We
             * only acquire an exclusive write latch on the leaf node from
             * which the key-value element needs to be removed.
             */

            root_latch_.LockExclusive();

            // Empty B+Tree
            if (root_ == nullptr) {
                root_latch_.UnlockExclusive();
                return false;
            }

            BaseNode *current = root_;
            BaseNode *parent = nullptr;

            current->GetNodeSharedLatch();
            while (current->GetType() != NodeType::LeafType) {
                if (parent != nullptr) {
                    parent->ReleaseNodeSharedLatch();
                } else {
                    root_latch_.UnlockExclusive();
                }

                parent = current;

                current = static_cast<InnerNode *>(current)->FindPivot(keyToRemove)->second;
                current->GetNodeSharedLatch();
            }

            current->ReleaseNodeSharedLatch();
            current->GetNodeExclusiveLatch();

            bool removable = false;
            auto node = static_cast<LeafNode *>(current);

            if (parent != nullptr) {
                parent->ReleaseNodeSharedLatch();
                removable = node->GetCurrentSize() > node->GetMinSize(); // underflow?
            } else {
                root_latch_.UnlockExclusive();
                // root node is also the leaf node
                removable = node->GetCurrentSize() > 1; // will root change?
            }

            auto iter = node->FindLocation(keyToRemove);
            if (removable) {
                // Key does not exist in the node
                if (iter == node->End()) {
                    current->ReleaseNodeExclusiveLatch();
                    return false;
                }

                // Does not match the key to be removed
                if (keyToRemove != iter->first) {
                    current->ReleaseNodeExclusiveLatch();
                    return false;
                }

                node->DeleteElement(iter);
                current->ReleaseNodeExclusiveLatch();
                return true;
            }

            current->ReleaseNodeExclusiveLatch();
            /**
             * Optimistic approach failed.
             */

            root_latch_.LockExclusive();
            bool holds_root_latch = true;

            current = root_;
            current->GetNodeExclusiveLatch();

            std::vector<BaseNode *> stack_latched_nodes{};
            while (current->GetType() != NodeType::LeafType) {
                auto node = static_cast<InnerNode *>(current);

                // Release all parent latches if this node is safe
                if (node->GetCurrentSize() > node->GetMinSize()) {
                    holds_root_latch = ReleaseAllWriteLatches(stack_latched_nodes, holds_root_latch);
                }

                stack_latched_nodes.push_back(current);

                current = node->FindPivot(keyToRemove)->second;
                current->GetNodeExclusiveLatch();
            }

            node = static_cast<LeafNode *>(current);
            iter = node->FindLocation(keyToRemove);

            node->DeleteElement(iter);

            /**
             * Verify underflow condition exists before proceeding
             * to rebalance the B+Tree. It is possible by the time
             * we abandoned the optimistic approach and retried the
             * node may have been rebalanced and will not underflow
             * anymore.
             */
            if (node->GetCurrentSize() >= node->GetMinSize()) {
                node->ReleaseNodeExclusiveLatch();
                ReleaseAllWriteLatches(stack_latched_nodes, holds_root_latch);

                return true;
            }

            /**
             * Rebalance the B+Tree
             */

            ElasticNode<KeyNodePointerPair> *inner_node{nullptr};
            bool deletion_finished = false;

            if (!stack_latched_nodes.empty()) {
                auto parent = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(*stack_latched_nodes.rbegin());
                stack_latched_nodes.pop_back();

                /**
                 * For re-balancing the tree the under-flowing leaf
                 * node can either borrow an element from a neighbour
                 * leaf node connected to the same parent inner node.
                 *
                 * We attempt to borrow one element from a neighbour
                 * leaf node as long as it does not lead to underflow
                 * of the neighbouring leaf node. If this is not
                 * possible we then try to merge this node with one
                 * of th neighbour leaf nodes.
                 */

                auto maybe_previous = static_cast<InnerNode *>(parent)->MaybePreviousWithSeparator(keyToRemove);
                if (maybe_previous.has_value()) {
                    auto other = reinterpret_cast<ElasticNode<KeyValuePair> *>((*maybe_previous).first);
                    auto pivot = (*maybe_previous).second;

                    other->GetNodeExclusiveLatch();

                    bool will_underflow = (other->GetCurrentSize() - 1) < static_cast<LeafNode *>(other)->GetMinSize();
                    if (!will_underflow) {
                        node->InsertElementIfPossible((*other->RBegin()), node->Begin());
                        other->PopEnd();
                        pivot->first = node->Begin()->first;

                        BPLUSTREE_ASSERT(node->GetCurrentSize() >= static_cast<LeafNode *>(node)->GetMinSize(),
                                         "node meets minimum occupancy requirement after borrow from previous leaf node");
                        BPLUSTREE_ASSERT(other->GetCurrentSize() >= static_cast<LeafNode *>(other)->GetMinSize(),
                                         "borrowing one element did not cause underflow in previous leaf node");

                        current->ReleaseNodeExclusiveLatch();
                        deletion_finished = true;
                    } else {
                        BPLUSTREE_ASSERT(other->GetCurrentSize() + node->GetCurrentSize() <= node->GetMaxSize(),
                                         "contents will fit a single leaf node after merge");
                        other->MergeNode(node);
                        if (node->GetSiblingRight() != nullptr) {
                            auto sibling_right = static_cast<LeafNode *>(node->GetSiblingRight());

                            sibling_right->GetNodeExclusiveLatch();
                            sibling_right->SetSiblingLeft(other);
                            sibling_right->ReleaseNodeExclusiveLatch();
                        }
                        other->SetSiblingRight(node->GetSiblingRight());

                        parent->DeleteElement(pivot);

                        current->ReleaseNodeExclusiveLatch();
                        node->FreeElasticNode();
                    }

                    other->ReleaseNodeExclusiveLatch();
                } else {
                    auto maybe_next = static_cast<InnerNode *>(parent)->MaybeNextWithSeparator(keyToRemove);
                    if (maybe_next.has_value()) {
                        auto other = reinterpret_cast<ElasticNode<KeyValuePair> *>((*maybe_next).first);
                        auto pivot = (*maybe_next).second;

                        other->GetNodeExclusiveLatch();

                        bool will_underflow =
                                (other->GetCurrentSize() - 1) < static_cast<LeafNode *>(other)->GetMinSize();
                        if (!will_underflow) {
                            node->InsertElementIfPossible((*other->Begin()), node->End());
                            other->PopBegin();
                            pivot->first = other->Begin()->first;

                            BPLUSTREE_ASSERT(node->GetCurrentSize() >= static_cast<LeafNode *>(node)->GetMinSize(),
                                             "node meets minimum occupancy requirement after borrow from previous leaf node");
                            BPLUSTREE_ASSERT(other->GetCurrentSize() >= static_cast<LeafNode *>(other)->GetMinSize(),
                                             "borrowing one element did not cause underflow in previous leaf node");

                            current->ReleaseNodeExclusiveLatch();
                            other->ReleaseNodeExclusiveLatch();

                            deletion_finished = true;
                        } else {
                            BPLUSTREE_ASSERT(other->GetCurrentSize() + node->GetCurrentSize() <= node->GetMaxSize(),
                                             "contents will fit a single leaf node after merge");
                            node->MergeNode(other);
                            if (other->GetSiblingRight() != nullptr) {
                                auto sibling_right = static_cast<LeafNode *>(other->GetSiblingRight());

                                sibling_right->GetNodeExclusiveLatch();
                                sibling_right->SetSiblingLeft(node);
                                sibling_right->ReleaseNodeExclusiveLatch();
                            }
                            node->SetSiblingRight(other->GetSiblingRight());

                            parent->DeleteElement(pivot);

                            other->ReleaseNodeExclusiveLatch();
                            other->FreeElasticNode();

                            current->ReleaseNodeExclusiveLatch();
                        }
                    }
                }

                if (parent->GetCurrentSize() >= static_cast<InnerNode *>(parent)->GetMinSize()) {
                    deletion_finished = true;
                }

                inner_node = parent;
            }

            if (deletion_finished) {
                inner_node->ReleaseNodeExclusiveLatch();
                holds_root_latch = ReleaseAllWriteLatches(stack_latched_nodes, holds_root_latch);

                return true;
            }

            // Re-balances tree
            while (!deletion_finished && !stack_latched_nodes.empty()) {
                auto parent = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(*stack_latched_nodes.rbegin());
                stack_latched_nodes.pop_back();

                auto maybe_previous = static_cast<InnerNode *>(parent)->MaybePreviousWithSeparator(keyToRemove);
                if (maybe_previous.has_value()) {
                    auto other = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>((*maybe_previous).first);
                    other->GetNodeExclusiveLatch();

                    auto pivot = (*maybe_previous).second;

                    bool will_underflow = (other->GetCurrentSize() - 1) < static_cast<InnerNode *>(other)->GetMinSize();
                    if (!will_underflow) {
                        auto borrowed = *(other->RBegin());
                        other->PopEnd();

                        inner_node->InsertElementIfPossible(
                                std::make_pair(pivot->first, inner_node->GetLowKeyPair().second),
                                inner_node->Begin());
                        inner_node->SetLowKeyPair(std::make_pair(pivot->first, borrowed.second));

                        pivot->first = borrowed.first;

                        inner_node->ReleaseNodeExclusiveLatch();
                        /**
                         *        (parent)
                         *       +-------------+
                         *       |...|Ky,Py|...|
                         *       +-------------+
                         *         /       \
                         *        /         \
                         *  +---------+    +--------+
                         *  |...|Kx,Px|    |_,Pl|...|
                         *  +---------+    +--------+
                         *   (previous)    (underflow node)
                         *
                         *
                         *        (parent)
                         *       +-----------------+
                         *       |...|  Kx,Py  |...|   <- pivot key updated
                         *       +-----------------+
                         *         /           \
                         *        /             \
                         *  +---------+        +--------------+
                         *  |...|     |<--+    |_,Px|Ky,Pl|...| <--+
                         *  +---------+   |    +--------------+    |
                         *                |                        +-- Low key node pointer updated
                         *                |                        +-- Old pivot key from parent and previous low key
                         *                |                            node pointer added as the new first element
                         *                |
                         *                +--- last element removed from previous node
                         */
                    } else {
                        other->InsertElementIfPossible(
                                std::make_pair(pivot->first, inner_node->GetLowKeyPair().second),
                                other->End()
                        );
                        other->MergeNode(inner_node);

                        parent->DeleteElement(pivot);

                        inner_node->ReleaseNodeExclusiveLatch();
                        inner_node->FreeElasticNode();
                    }

                    other->ReleaseNodeExclusiveLatch();
                } else {
                    auto maybe_next = static_cast<InnerNode *>(parent)->MaybeNextWithSeparator(keyToRemove);
                    if (maybe_next.has_value()) {
                        auto other = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>((*maybe_next).first);
                        auto pivot = (*maybe_next).second;

                        other->GetNodeExclusiveLatch();

                        bool will_underflow =
                                (other->GetCurrentSize() - 1) < static_cast<InnerNode *>(other)->GetMinSize();
                        if (!will_underflow) {
                            auto borrowed = *(other->Begin());
                            other->PopBegin();

                            inner_node->InsertElementIfPossible(
                                    std::make_pair(pivot->first, other->GetLowKeyPair().second),
                                    inner_node->End()
                            );
                            other->SetLowKeyPair(std::make_pair(pivot->first, borrowed.second));
                            pivot->first = borrowed.first;

                            other->ReleaseNodeExclusiveLatch();
                            inner_node->ReleaseNodeExclusiveLatch();
                            /**
                             *        (parent)
                             *       +-------------+
                             *       |...|Ky,Py|...|
                             *       +-------------+
                             *        /       \
                             *       /         \
                             *  +-------+    +--------------------+
                             *  |...|...|    |_,Pl|Ko,Po|Ki,Pi|...|
                             *  +-------+    +--------------------+
                             *  (underflow) (next node)
                             *
                             *
                             *             (parent)
                             *            +-------------+
                             *            |...|Ko,Py|...|
                             *            +-------------+
                             *             /      \
                             *           /         \
                             *  +---------+       +--------------+
                             *  |...|Ky,Pl|<--+   |_,Po|Ki,Pi|...|<--+
                             *  +---------+   |   +--------------+   |
                             *                |                      |
                             *                |                      +-- Low key node pointer removed
                             *                |                      +-- First node pointer moved to low key node pointer
                             *                |
                             *                +-- Old Pivot + Low key node pointer from next node added as last element
                             */
                        } else {
                            inner_node->InsertElementIfPossible(
                                    std::make_pair(pivot->first, other->GetLowKeyPair().second),
                                    inner_node->End()
                            );
                            inner_node->MergeNode(other);

                            parent->DeleteElement(pivot);

                            other->ReleaseNodeExclusiveLatch();
                            other->FreeElasticNode();

                            inner_node->ReleaseNodeExclusiveLatch();
                        }
                    }
                }

                if (parent->GetCurrentSize() >= static_cast<InnerNode *>(parent)->GetMinSize()) {
                    deletion_finished = true;
                }

                inner_node = parent;
            }

            if (deletion_finished) {
                inner_node->ReleaseNodeExclusiveLatch();
                holds_root_latch = ReleaseAllWriteLatches(stack_latched_nodes, holds_root_latch);

                return true;
            }

            /**
             * Reduce tree depth if root node has insufficient children
             */
            if (!deletion_finished && inner_node != nullptr) {
                BPLUSTREE_ASSERT(holds_root_latch, "Exclusive root latch held");
                BPLUSTREE_ASSERT(inner_node == root_, "delete returned back to root node");

                if (inner_node->GetCurrentSize() == 0) {
                    auto old_root = root_;
                    root_ = inner_node->GetLowKeyPair().second;

                    inner_node->ReleaseNodeExclusiveLatch();

                    static_cast<InnerNode *>(old_root)->FreeElasticNode();
                } else {
                    inner_node->ReleaseNodeExclusiveLatch();
                }

                root_latch_.UnlockExclusive();

                return true;
            }

            /**
             * Update root if the B+Tree has no more elements left
             */
            if (!deletion_finished && inner_node == nullptr && node == root_) {
                BPLUSTREE_ASSERT(holds_root_latch, "Has exclusive latch for modifying root");

                if (node->GetCurrentSize() == 0) {
                    node->FreeElasticNode();

                    root_ = nullptr;
                    root_latch_.UnlockExclusive();
                } else {
                    node->ReleaseNodeExclusiveLatch();

                    root_latch_.UnlockExclusive();
                }
            }

            return true;
        }

        std::string ToGraph() {
            if (root_ == nullptr) {
                return "digraph empty_bplus_tree {}";
            }

            std::stringstream graph;
            std::queue<BaseNode *> nodes;
            std::deque<std::pair<std::string, std::string>> edges;
            std::deque<std::pair<std::string, std::string>> leaf_edges;

            graph << "digraph bplus_tree {\n";

            nodes.push(root_);
            while (!nodes.empty()) {
                auto current_node = nodes.front();
                nodes.pop();

                if (current_node->GetType() == NodeType::InnerType) {
                    auto inner_node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(current_node);

                    graph << std::endl;
                    graph << MakeNodeIdFor(inner_node);
                    graph << " [";
                    graph << " shape=" << WrapInDoubleQuotes("plaintext");
                    graph << " label=<" << ToHTMLTable(inner_node) << ">";
                    graph << " fillcolor=" << WrapInDoubleQuotes("#F3B664") << "style=" << WrapInDoubleQuotes("filled");
                    graph << " ]";
                    graph << std::endl;

                    edges.push_front(std::make_pair(MakeNodeIdFor(inner_node) + ":low_key",
                                                    MakeNodeIdFor(inner_node->GetLowKeyPair().second)));
                    nodes.push(inner_node->GetLowKeyPair().second);

                    for (int i = 0; i < inner_node->GetCurrentSize(); ++i) {
                        auto child_node = inner_node->At(i).second;

                        edges.push_front(std::make_pair(MakeNodeIdFor(inner_node) + ":key_" + std::to_string(i),
                                                        MakeNodeIdFor(child_node) + ":n"));
                        nodes.push(child_node);
                    }
                } else {
                    auto leaf_node = reinterpret_cast<ElasticNode<KeyValuePair> *>(current_node);

                    graph << std::endl;
                    graph << MakeNodeIdFor(leaf_node);
                    graph << " [";
                    graph << " shape=" << WrapInDoubleQuotes("plaintext");
                    graph << " label=<" << ToHTMLTable(leaf_node) << ">";
                    graph << " fillcolor=" << WrapInDoubleQuotes("#9FBB73") << "style=" << WrapInDoubleQuotes("filled");
                    graph << " ]";
                    graph << std::endl;

                    if (leaf_node->GetSiblingRight() != nullptr) {
                        leaf_edges.push_front(
                                std::make_pair(MakeNodeIdFor(leaf_node),
                                               MakeNodeIdFor(leaf_node->GetSiblingRight())));
                    }
                }
            }
            graph << std::endl;

            while (!edges.empty()) {
                auto edge = edges.back();
                edges.pop_back();

                graph << edge.first << " -> " << edge.second << std::endl;
            }
            graph << std::endl;

            if (!leaf_edges.empty()) {
                graph << "subgraph leaf_nodes {" << std::endl;
                std::deque<std::string> leaf_node_ids;
                while (!leaf_edges.empty()) {
                    auto edge = leaf_edges.back();
                    leaf_edges.pop_back();

                    graph << edge.first << " -> " << edge.second << std::endl;
                    graph << edge.second << " -> " << edge.first << std::endl;

                    // Ensures that duplicate node identifiers are not added to the
                    // deque by checking the current edge with the last edge which
                    // was added in the previous iteration.
                    if (leaf_node_ids.empty() || leaf_node_ids.back() != edge.first) {
                        leaf_node_ids.push_back(edge.first);
                    }
                    leaf_node_ids.push_back(edge.second);
                }
                graph << std::endl;

                graph << "{" << std::endl;
                graph << "rank=" << WrapInDoubleQuotes("same") << std::endl;
                for (auto &leaf_id: leaf_node_ids) {
                    graph << leaf_id << std::endl;
                }
                graph << "}" << std::endl;

                graph << "}" << std::endl; // end subgraph leaf_nodes
            }

            graph << "}\n";
            return graph.str();
        }

    private:
        std::string MakeNodeIdFor(BaseNode *node) {
            std::string node_prefix = "Node_";
            std::stringstream out;
            out << node_prefix << reinterpret_cast<std::uintptr_t>(node);
            return out.str();
        }

        std::string WrapInDoubleQuotes(std::string s) {
            std::string dq = "\"";
            std::stringstream out;
            out << dq << s << dq;
            return out.str();
        }

        std::string ToHTMLTable(ElasticNode<KeyNodePointerPair> *inner_node) {
            std::stringstream table;
            auto colspan = std::to_string(inner_node->GetCurrentSize() + 1);

            table << "<table cellspacing=" << WrapInDoubleQuotes("2");
            table << " cellborder=" << WrapInDoubleQuotes("2");
            table << " border=" << WrapInDoubleQuotes("0");
            table << ">" << std::endl;

            table << "<tr>" << "<td colspan=" << WrapInDoubleQuotes(colspan) << ">"
                  << "count: " << inner_node->GetCurrentSize()
                  << "</td></tr>" << std::endl;

            table << "<tr>" << std::endl;
            table << "<td port=" << WrapInDoubleQuotes("low_key") << ">low key:" << inner_node->GetLowKeyPair().first
                  << "</td>" << std::endl;
            for (int i = 0; i < inner_node->GetCurrentSize(); ++i) {
                table << "<td port=" << WrapInDoubleQuotes("key_" + std::to_string(i)) << ">"
                      << inner_node->At(i).first
                      << "</td>" << std::endl;
            }
            table << "</tr>" << std::endl;

            table << "</table>" << std::endl;

            return table.str();
        }

        std::string ToHTMLTable(ElasticNode<KeyValuePair> *leaf_node) {
            std::stringstream table;
            auto colspan = std::to_string(leaf_node->GetCurrentSize());

            table << "<table cellspacing=" << WrapInDoubleQuotes("2");
            table << " cellborder=" << WrapInDoubleQuotes("2");
            table << " border=" << WrapInDoubleQuotes("0");
            table << ">" << std::endl;

            table << "<tr>" << "<td colspan=" << WrapInDoubleQuotes(colspan) << ">"
                  << "count: " << leaf_node->GetCurrentSize()
                  << "</td></tr>" << std::endl;

            table << "<tr>" << std::endl;
            for (int i = 0; i < leaf_node->GetCurrentSize(); ++i) {
                table << "<td port=" << WrapInDoubleQuotes("key_" + std::to_string(i)) << ">"
                      << leaf_node->At(i).first
                      << "</td>" << std::endl;
            }
            table << "</tr>" << std::endl;

            table << "</table>" << std::endl;

            return table.str();
        }

    private:
        BaseNode *root_;
        SharedLatch root_latch_;
        int inner_node_max_size_;
        int leaf_node_max_size_;
    };

}
#endif //BPLUSTREE_H
