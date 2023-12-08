#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <utility>
#include <vector>
#include <sstream>
#include <queue>
#include <optional>
#include "macros.h"

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

    private:
        NodeType type_;
        int max_size_;
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
                return std::make_tuple(GetLowKeyPair().second, pivot);
            }

            return std::make_tuple(std::prev(pivot)->second, pivot);
        }

        std::optional<std::pair<BaseNode *, KeyNodePointerPair *>> MaybeNextWithSeparator(int search_key) {
            auto pivot = FindPivot(search_key);

            // Right most element, has no next element
            if (std::next(pivot) == End()) {
                return {};
            }

            if (pivot->second == GetLowKeyPair().second) {
                return std::make_tuple(Begin()->second, Begin());
            }

            return std::make_tuple(std::next(pivot)->second, std::next(pivot));

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
                SetEndIterator();
                return;
            }

            current_node_ = static_cast<ElasticNode<KeyValuePair> *>(current_node_->GetSiblingRight());
            current_element_ = current_node_->Begin();
        }

        void operator--() {
            BPLUSTREE_ASSERT(state_ == VALID, "Iterator in invalid state");

            if (current_element_ != current_node_->Begin()) {
                current_element_ = std::prev(current_element_);
                return;
            }

            if (current_node_->GetSiblingLeft() == nullptr) {
                SetREndIterator();
                return;
            }
            current_node_ = static_cast<ElasticNode<KeyValuePair> *>(current_node_->GetSiblingLeft());
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

    private:
        enum IteratorState {
            VALID, INVALID, END, REND
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
            if (root_ == nullptr) { return nullptr; }

            BaseNode *current_node = root_;
            while (current_node->GetType() != NodeType::LeafType) {
                auto node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(current_node);
                current_node = node->GetLowKeyPair().second;
            }

            return current_node;
        }

        BaseNode *FindLeafNode(int key) {
            if (root_ == nullptr) { return nullptr; }

            BaseNode *current_node = root_;
            while (current_node->GetType() != NodeType::LeafType) {
                auto node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(current_node);
                auto iter = static_cast<InnerNode *>(node)->FindLocation(key);

                if (iter == node->End()) {
                    current_node = std::prev(iter)->second;
                } else if (key == iter->first) {
                    current_node = iter->second;
                } else {
                    current_node = (iter != node->Begin()) ? std::prev(iter)->second : node->GetLowKeyPair().second;
                }
            }

            return current_node;
        }

        BaseNode *FindLastLeafNode() {
            if (root_ == nullptr) { return nullptr; }

            BaseNode *current_node = root_;
            while (current_node->GetType() != NodeType::LeafType) {
                auto node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(current_node);
                current_node = std::prev(node->End())->second;
            }

            return current_node;
        }

        std::optional<int> FindValueOfKey(int key) {
            auto current_node = FindLeafNode(key);
            if (current_node == nullptr) {
                return std::nullopt;
            }

            auto node = reinterpret_cast<ElasticNode<KeyValuePair> *>(current_node);
            auto iter = static_cast<LeafNode *>(node)->FindLocation(key);

            if (iter == node->End() || key != iter->first) {
                return std::nullopt;
            }

            return std::optional<int>{iter->second};
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

            std::cout << "Releasing nodes: " << free_queue.size() << std::endl;
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


        /**
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
            if (root_ == nullptr) {
                // Create an empty leaf node, which is also the root
                KeyNodePointerPair dummy_low_key = std::make_pair(element.first, nullptr);
                root_ = ElasticNode<KeyValuePair>::Get(NodeType::LeafType, dummy_low_key, leaf_node_max_size_);
            }

            BaseNode *current_node = root_;

            // Used to maintain a stack of node pointers from root to leaf
            std::vector<BaseNode *> node_search_path{};

            // Search for leaf node traversing down the B+Tree
            while (current_node->GetType() != NodeType::LeafType) {
                auto node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(current_node);
                node_search_path.push_back(current_node);

                auto iter = static_cast<InnerNode *>(node)->FindLocation(element.first);
                if (iter == node->End()) {
                    current_node = std::prev(iter)->second;
                } else if (element.first == iter->first) {
                    current_node = iter->second;
                } else { // element.first < iter->first
                    current_node = (iter != node->Begin()) ? std::prev(iter)->second : node->GetLowKeyPair().second;
                }
            }

            // Reached the leaf node
            auto node = reinterpret_cast<ElasticNode<KeyValuePair> *>(current_node);

            // Find where to insert element in leaf node
            auto iter = static_cast<LeafNode *>(node)->FindLocation(element.first);

            // Does not insert if this is a duplicate key
            if (iter != node->End() && element.first == iter->first) {
                return false;
            }

            // Insert will finish if the leaf node has space
            if (node->InsertElementIfPossible(element, iter)) {
                return true;
            }

            // Split the leaf node and insert the element in one of
            // the leaf nodes
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
            }

            if (node->GetSiblingRight() != nullptr) {
                auto current_right_sibling = reinterpret_cast<ElasticNode<KeyValuePair> *>(node->GetSiblingRight());
                current_right_sibling->SetSiblingLeft(split_node);
            }
            // Fix sibling pointers to maintain a bidirectional chain
            // of leaf nodes
            split_node->SetSiblingLeft(node);
            split_node->SetSiblingRight(node->GetSiblingRight());
            node->SetSiblingRight(split_node);

            bool insertion_finished = false;

            KeyNodePointerPair inner_node_element = std::make_pair(split_node->Begin()->first, split_node);
            while (!insertion_finished && !node_search_path.empty()) {
                auto inner_node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(*node_search_path.rbegin());
                node_search_path.pop_back();

                if (inner_node->InsertElementIfPossible(
                        inner_node_element,
                        static_cast<InnerNode *>(inner_node)->FindLocation(inner_node_element.first)
                )) {
                    // we are done if insertion is possible without further splitting
                    insertion_finished = true;
                } else {
                    // Recursively split the node
                    auto split_inner_node = inner_node->SplitNode();
                    if (inner_node_element.first >= split_inner_node->Begin()->first) {
                        split_inner_node->InsertElementIfPossible(inner_node_element,
                                                                  static_cast<InnerNode *>(split_inner_node)->FindLocation(
                                                                          inner_node_element.first));
                    } else {
                        inner_node->InsertElementIfPossible(inner_node_element,
                                                            static_cast<InnerNode *>(inner_node)->FindLocation(
                                                                    inner_node_element.first));
                    }
                    split_inner_node->GetLowKeyPair().first = split_inner_node->Begin()->first;
                    split_inner_node->GetLowKeyPair().second = split_inner_node->Begin()->second;
                    split_inner_node->PopBegin();

                    inner_node_element = std::make_pair(split_inner_node->GetLowKeyPair().first, split_inner_node);
                }
            }

            // If insertion has not finished by now the split propagated
            // back up to the root. So we now need to split the root node
            // as well
            if (!insertion_finished) {
                auto old_root = root_;

                KeyNodePointerPair low_key = std::make_pair(inner_node_element.first, old_root);
                root_ = ElasticNode<KeyNodePointerPair>::Get(NodeType::InnerType, low_key, inner_node_max_size_);

                auto new_root_node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(root_);
                new_root_node->InsertElementIfPossible(inner_node_element,
                                                       static_cast<InnerNode *>(new_root_node)->FindLocation(
                                                               inner_node_element.first));
            }

            return true;
        }

        bool Delete(const KeyValuePair &element) {
            if (root_ == nullptr) { return false; }

            // Find the leaf node that contains key-value element
            BaseNode *current = root_;
            std::vector<BaseNode *> stack{};

            while (current->GetType() != NodeType::LeafType) {
                auto node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(current);

                stack.push_back(node);

                current = static_cast<InnerNode *>(node)->FindPivot(element.first)->second;
            }

            auto node = reinterpret_cast<ElasticNode<KeyValuePair> *>(current);
            auto iter = static_cast<LeafNode *>(node)->FindLocation(element.first);

            if (iter == node->End() || iter->first != element.first) {
                return false; // key was not found in the tree
            }

            node->DeleteElement(iter);

            // Check for underflow
            if (node->GetCurrentSize() >= static_cast<LeafNode *>(node)->GetMinSize()) {
                return true;
            }

            ElasticNode<KeyNodePointerPair> *inner_node{nullptr};
            if (!stack.empty()) {
                auto parent = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(*stack.rbegin());
                stack.pop_back();

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

                auto maybe_previous = static_cast<InnerNode *>(parent)->MaybePreviousWithSeparator(element.first);
                if (maybe_previous.has_value()) {
                    auto other = reinterpret_cast<ElasticNode<KeyValuePair> *>((*maybe_previous).first);
                    auto pivot = (*maybe_previous).second;

                    bool will_underflow = (other->GetCurrentSize() - 1) < static_cast<LeafNode *>(other)->GetMinSize();
                    if (!will_underflow) {
                        node->InsertElementIfPossible((*other->RBegin()), node->Begin());
                        other->PopEnd();
                        pivot->first = node->Begin()->first;

                        BPLUSTREE_ASSERT(node->GetCurrentSize() >= static_cast<LeafNode *>(node)->GetMinSize(),
                                         "node meets minimum occupancy requirement after borrow from previous leaf node");
                        BPLUSTREE_ASSERT(other->GetCurrentSize() >= static_cast<LeafNode *>(other)->GetMinSize(),
                                         "borrowing one element did not cause underflow in previous leaf node");

                        return true;
                    } else {
                        BPLUSTREE_ASSERT(other->GetCurrentSize() + node->GetCurrentSize() <= node->GetMaxSize(),
                                         "contents will fit a single leaf node after merge");
                        other->MergeNode(node);
                        if (node->GetSiblingRight() != nullptr) {
                            static_cast<LeafNode *>(node->GetSiblingRight())->SetSiblingLeft(other);
                        }
                        other->SetSiblingRight(node->GetSiblingRight());

                        parent->DeleteElement(pivot);
                        node->FreeElasticNode();
                    }
                } else {
                    auto maybe_next = static_cast<InnerNode *>(parent)->MaybeNextWithSeparator(element.first);
                    if (maybe_next.has_value()) {
                        auto other = reinterpret_cast<ElasticNode<KeyValuePair> *>((*maybe_next).first);
                        auto pivot = (*maybe_next).second;

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

                            return true;
                        } else {
                            BPLUSTREE_ASSERT(other->GetCurrentSize() + node->GetCurrentSize() <= node->GetMaxSize(),
                                             "contents will fit a single leaf node after merge");
                            node->MergeNode(other);
                            if (other->GetSiblingRight() != nullptr) {
                                static_cast<LeafNode *>(other->GetSiblingRight())->SetSiblingLeft(node);
                            }
                            node->SetSiblingRight(other->GetSiblingRight());

                            parent->DeleteElement(pivot);
                            other->FreeElasticNode();
                        }
                    }
                }

                if (parent->GetCurrentSize() >= static_cast<InnerNode *>(parent)->GetMinSize()) {
                    return true;
                }

                inner_node = parent;
            }

            // Re-balances tree
            while (!stack.empty()) {
                auto parent = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(*stack.rbegin());
                stack.pop_back();

                auto maybe_previous = static_cast<InnerNode *>(parent)->MaybePreviousWithSeparator(element.first);
                if (maybe_previous.has_value()) {
                    auto other = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>((*maybe_previous).first);
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
                        inner_node->FreeElasticNode();
                    }
                } else {
                    auto maybe_next = static_cast<InnerNode *>(parent)->MaybeNextWithSeparator(element.first);
                    if (maybe_next.has_value()) {
                        auto other = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>((*maybe_previous).first);
                        auto pivot = (*maybe_previous).second;

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
                        }
                    }
                }

                if (parent->GetCurrentSize() >= static_cast<InnerNode *>(parent)->GetMinSize()) {
                    return true;
                }

                inner_node = parent;
            }

            // Inner node underflow. Tree re-balance needed.


            return false;
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
        int inner_node_max_size_;
        int leaf_node_max_size_;
    };

}
#endif //BPLUSTREE_H
