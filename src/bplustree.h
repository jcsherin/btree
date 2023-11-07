#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <utility>
#include <vector>
#include "macros.h"

namespace bplustree {
    enum class NodeType : int {
        InnerType = 0, LeafType = 1
    };

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

    template<typename ElementType>
    class ElasticNode : public BaseNode {
    public:
        ElasticNode(NodeType p_type, BaseNode *p_sibling_left, BaseNode *p_sibling_right, int p_max_size) :
                BaseNode(p_type, p_max_size),
                sibling_left_{p_sibling_left},
                sibling_right_{p_sibling_right},
                current_size_{0} {}

        ElasticNode *SplitNode() {
            // Split the node only when the addition of one more key-value
            // pair will make the node full
            if (!this->WillBeFullAfterInsert()) {
                return nullptr;
            }

            ElasticNode *new_node = this->Get(this->GetType(), this->sibling_left_, this->sibling_right_,
                                              this->GetMaxSize());

            std::cout << "(pre copy) New node, current size: " << new_node->GetCurrentSize() << std::endl;
            std::cout << "(pre copy) Old node, current size: " << this->GetCurrentSize() << std::endl;

            int copy_from_index = FastCeilIntDivision(this->GetCurrentSize(), 2);
            int copied_count{0};
            for (int i = 0, j = copy_from_index; j < this->GetCurrentSize(); ++i, ++j) {
                std::cout << "Copy from: " << j << " to: " << i << " key: " << this->start_[j].first << std::endl;
                new_node->start_[i] = this->start_[j];

                // Adjust array size after copying
                ++copied_count;
            }
            new_node->current_size_ = new_node->current_size_ + copied_count;
            this->current_size_ = this->current_size_ - copied_count;
            std::cout << "copied count: " << copied_count << std::endl;
            std::cout << "(post copy) New node, current size: " << new_node->GetCurrentSize() << std::endl;
            std::cout << "(post copy) Old node, current size: " << this->GetCurrentSize() << std::endl;

            return new_node;
        }

        /**
         *
         * @return true if adding one more element will make the node full and
         * therefore it has to split, false otherwise
         */
        bool WillBeFullAfterInsert() {
            return (this->GetCurrentSize() + 1 >= this->GetMaxSize());
        }

        /**
         * Static helper to allocate storage for an elastic node.
         */
        static ElasticNode *Get(NodeType p_type, BaseNode *p_sibling_left, BaseNode *p_sibling_right, int p_max_size) {
            auto *alloc = new char[sizeof(ElasticNode) + p_max_size * sizeof(ElementType)];

            /**
             * Construct elastic node in allocated storage
             * https://en.cppreference.com/w/cpp/language/new#Placement_new
             */
            auto elastic_node = reinterpret_cast<ElasticNode *>(alloc);
            new(elastic_node) ElasticNode(p_type, p_sibling_left, p_sibling_right, p_max_size);

            return elastic_node;
        }

        /**
         * Free elastic node
         */
        void FreeElasticNode() {
            ElasticNode *alloc = this;
            delete[] reinterpret_cast<char *>(alloc);
        }

        ElementType *Begin() { return start_; }

        ElementType *End() { return Begin() + GetCurrentSize(); }

        int GetOffset(ElementType *location) { return location - Begin(); }

        int GetCurrentSize() { return current_size_; }

        // assert index < max_size_
        ElementType &At(const int index) { return *(Begin() + index); }

        bool InsertElementIfPossible(ElementType element, const int index) {
            // This guard could be removed if it can be guaranteed that this
            // method will not be called on a node which is full in the
            // B+Tree insert method
            if (GetCurrentSize() >= GetMaxSize()) {
                return false;
            }
            /**
             * To insert an element at the given index all the elements have
             * to be relocated to the right by 1. To copy the elements over
             * correctly, the traversal should begin from the end of the
             * internal array.
             *
             * TODO: Maybe use `std::memmove` here instead!
             */
            for (int i = GetCurrentSize() - 1; i >= index; --i) {
                start_[i + 1] = start_[i];
            }

            // Place the element at index
            new(start_ + index) ElementType{element};
            current_size_ = current_size_ + 1;

            return true;
        }

        BaseNode *GetSiblingLeft() { return sibling_left_; }

        BaseNode *GetSiblingRight() { return sibling_right_; }

        void SetSiblingLeft(BaseNode *node) { sibling_left_ = node; }

        void SetSiblingRight(BaseNode *node) { sibling_right_ = node; }

    private:
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

    private:
        // Sibling pointers for chaining leaf nodes
        BaseNode *sibling_left_;
        BaseNode *sibling_right_;

        // Size of the internal array `start_`
        int current_size_;

        /*
         * Struct hack (flexible array member)
         * https://developers.redhat.com/articles/2022/09/29/benefits-limitations-flexible-array-members
         *
         * The array for holding the key-value elements
         */
        ElementType start_[0];
    };

    using KeyNodePointerPair = std::pair<int, BaseNode *>;

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

            /*
             * The inner node contains N node pointers, and N-1 keys. We
             * store the extra node pointer at index 0. The first valid
             * key is stored beginning at index 1. So for key lookups we
             * should always skip index 0.
             *
             *      +----+-------+-----+-------+
             *      | P0 | K1+P1 | ... | KN+PN |
             *      +----+-------+-----+-------+
             */
            auto first = this->Begin() + 1;
            KeyNodePointerPair *iter = std::lower_bound(
                    first, this->End(), dummy_element,
                    [](const auto &a, const auto &b) { return a < b; }
            );

            return iter;
        }
    };

    using KeyValuePair = std::pair<int, int>;

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

        int FindLocation(const int key) {
            KeyValuePair dummy = std::make_pair(key, 0);
            KeyValuePair *iter = std::lower_bound(this->Begin(), this->End(), dummy,
                                                  [](const auto &a, const auto &b) {
                                                      return a < b;
                                                  });
            return this->GetOffset(iter);
        }
    };

    class BPlusTree {
    public:
        explicit BPlusTree(int p_inner_node_max_size, int p_leaf_node_max_size) :
                root_{nullptr},
                inner_node_max_size_{p_inner_node_max_size},
                leaf_node_max_size_{p_leaf_node_max_size} {}

        ~BPlusTree() {
            /**
             * TODO: Free all the nodes in the tree
             */
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
            /**
             * Sequential algorithm (no crab latching)
             * ---------------------------------------
             *
             * If B+Tree is empty:
             *      root_ = new leaf node
             *     __Invariant__:
             *          root_ != nullptr
             *          root_ == NodeType::LeafType
             * else (B+Tree is not empty):
             *      Find leaf node to insert key-value:
             *          __Invariant__:
             *              root != nullptr
             *              current_node = root_
             *
             *          Initialize an empty stack for maintaining node pointers:
             *              node_stack
             *
             *          while (current_node != NodeType::LeafType):
             *              __Invariant__:
             *                  current_node == NodeType::InnerType
             *
             *              node_stack.push(current_node)
             *
             *              find lower_bound for key in current_node
             *              if index == End(): (key greater than all other keys in this node)
             *                  traverse right most node pointer:
             *                      current_node = current_node->storage[size - 1]->node_pointer
             *              if key < current_node->storage[lower]->key:
             *                  traverse left node pointer:
             *                      current_node = current_node->storage[lower - 1]->node_pointer
             *              if key == current_node->storage[lower]->key:
             *                  traverse current (right) node pointer:
             *                      current_node = current_node->storage[lower]->node_pointer
             *     __Invariant__:
             *          current_node == NodeType::LeafType
             *
             * Check if this key already exists in the leaf node:
             *      find lower_bound for key in current_node
             *      if key == current_node->storage[lower]->key:
             *          return false (reason: duplicate key)
             *     __Invariant__:
             *          key is NOT a duplicate
             *
             * If leaf node does NOT have to split for this key-value INSERT:
             *      insert key-value in current_node:
             *          find lower_bound for key in current_node
             *          if index == End():
             *              insert key-value pair as the last element in node
             *          if key < current_node->storage[lower]->key:
             *              copy/shift key-value pairs from lower to the end by 1 index
             *              this makes space for inserting key-value at index lower
             *              insert key-value at current_node->storage[lower]
             *          increment current_node.current_size by 1
             *      return true (Simple Insert)
             *      __Invariant__:
             *          key is guaranteed to be unique in the current_node
             *          after insert:
             *              current_node.current_size <= current_node.max_size - 1
             *
             * __Invariant__:
             *      current_node.current_size + 1 == current_node.max_size
             *      (condition for splitting the leaf node)
             *
             * Split leaf current_node:
             *      create leaf node - new_node
             *          min_size = ⌈(fanout - 1) / 2⌉
             *          max_size = (fanout - 1)
             *      copy half the key-value pairs from current_node to new_node
             *          beginning at index min_size up to the end:
             *          new_node.current_size += count_moved_key_values
             *      erase copied key-value pairs from current_node
             *          current_node.current_size -= count_moved_key_values
             *      __Invariant__:
             *          // each leaf node contains at least min_size key-values
             *          leaf_node.current_size >= min_size ⌈(fanout - 1) / 2⌉
             *          new_node.current_size >= min_size ⌈(fanout - 1) / 2⌉
             *          // total key-values remain the same after redistribution
             *
             *     Insert the key-value in either the current_node or new_node:
             *         If key < smallest key in new_node:
             *              insert key-value in current_node (same steps as earlier)
             *         else:
             *              insert key-value in new_node (same steps as earlier)
             *
             *     Insert new_node into the leaf chain:
             *          new_node.next_ = current_node.next_
             *          new_node.prev_ = current_node
             *          current_node.next_ = new_node
             *     __Invariant__:
             *          new_node.prev_ != nullptr (= current_node)
             *          current_node.next_ != nullptr (= new_node)
             *
             *  Set insertion_finished = false
             *  while (!insertion_finished && !node_stack.empty()):
             *       parent_node = node_stack.pop()
             *       smallest_key = new_node->GetFirstKey()
             *       key_node_pointer_pair = (smallest_key, new_node)
             *
             *       __Invariant__:
             *           key_node_pointer_pair.first = new_node->GetFirstKey()
             *           key_node_pointer_pair.second = new_node
             *
             *       idx = find index of current_node in parent_node
             *       if parent_node.size < parent_node.max_size:
             *           insert key_node_pointer_pair at (idx + 1) after current_node:
             *               relocate existing key-value pairs from (idx + 1) by 1 index to the right
             *               insert key_node_pointer_pair at (idx + 1)
             *           set insertion_finished = true
             *       else split inner parent_node:
             *          create inner node - split_inner_node
             *              min_size = ⌈fanout/ 2⌉ - 1
             *              max_size = fanout
             *          copy half the key-value pairs from parent_node to split_inner_node
             *              copy from index (min_size + 1):
             *                  takes into account the first stored key is invalid
             *              split_inner_node.current_size += count_moved_key_values
             *          erase moved key-value pairs from parent_node
             *              parent_node.current_size -= count_moved_key_values
             *          __Invariant__:
             *              each inner node contains at least min_size key-values
             *              total count of key-values have not changed after split
             *
             *          // assumes moved values from index (min_size + 1) onwards
             *          // to newly created inner node
             *          if idx > min_size: (insert in split_inner_node)
             *              idx = idx - min_size
             *              insert key_node_pointer_pair at (idx + 1) after current_node in split_inner_node:
             *                  relocate existing key-value pairs from (idx + 1) by 1 index to the right
             *                  insert key_node_pointer_pair at (idx + 1) in split_inner_node
             *              __Invariant__:
             *                  current_node pointer moved to split_inner_node
             *                  split_inner_node->storage[idx]->node_pointer == current_node
             *                  split_inner_node->storage[idx+1]->node_pointer = new_node
             *         else: (insert in parent_node)
             *              insert key_node_pointer_pair at (idx + 1) after current_node in parent_node:
             *                  relocate existing key-value pairs from (idx + 1) by 1 index to the right
             *                  insert key_node_pointer_pair at (idx + 1) in parent_node
             *              __Invariant__:
             *                  current_node pointer in parent_node
             *                  parent_node->storage[idx]->node_pointer == current_node
             *                  parent_node->storage[idx+1]->node_pointer = new_node
             *
             *         // Unwind the stack to insert split_inner_node in its parent
             *         current_node = parent_node
             *         new_node = split_inner_node
             *
             *  if (!insertion_finished):
             *      __Invariant__:
             *          root node has to be split
             *      Create a new root node containing the following key-value items:
             *          (smallest_key_current_node, current_node)
             *          (smallest_key_new_node, new_node)
             *      Make this the new root of the B+tree
             *      Set insertion_finished = true
             *
             * return true
             */
            if (root_ == nullptr) {
                // Create an empty leaf node, which is also the root
                root_ = ElasticNode<KeyValuePair>::Get(NodeType::LeafType, nullptr, nullptr, leaf_node_max_size_);
            }

            BaseNode *current_node = root_;
            std::vector<BaseNode *> node_write_path{}; // stack of node pointers

            // Traversing down the tree to find the leaf node where the
            // key-value element can be inserted
            while (current_node->GetType() != NodeType::LeafType) {
                auto node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(current_node);
                node_write_path.push_back(current_node);

                auto iter = static_cast<InnerNode *>(node)->FindLocation(element.first);
                if (iter == node->End()) {
                    auto last_element = (iter - 1);
                    current_node = std::prev(iter)->second;
                } else if (element.first == iter->first) {
                    current_node = iter->second;
                } else { // element.first < iter->first
                    current_node = std::prev(iter)->second;
                }
            }

            auto node = reinterpret_cast<ElasticNode<KeyValuePair> *>(current_node);
            auto index_greater_key_leaf = static_cast<LeafNode *>(node)->FindLocation(element.first);
            // Proceed further only if this is not an already existing key
            if (element.first == node->At(index_greater_key_leaf).first) {
                return false;
            }
            if (!node->WillBeFullAfterInsert() && node->InsertElementIfPossible(element, index_greater_key_leaf)) {
                return true;
            }

            // Leaf node has to split
            auto splitted_node = node->SplitNode();
            auto splitted_node_begin = splitted_node->Begin();
            if (splitted_node_begin->first > element.first) {
                auto insert_location = static_cast<LeafNode *>(node)->FindLocation(element.first);
                node->InsertElementIfPossible(element, insert_location);

                std::cout << "Inserted key: " << element.first << " at: " << insert_location
                          << " in older(of split) node." << std::endl;
                node->InsertElementIfPossible(element, static_cast<LeafNode *>(node)->FindLocation(element.first));
            } else {
                auto insert_location = static_cast<LeafNode *>(splitted_node)->FindLocation(element.first);

                std::cout << "Inserted key: " << element.first << " at: " << insert_location
                          << " in newly split node." << std::endl;
            }

            // Set sibling pointers correctly
            splitted_node->SetSiblingLeft(node);
            splitted_node->SetSiblingRight(node->GetSiblingRight());
            node->SetSiblingRight(splitted_node);

            // Create a new root node
            root_ = ElasticNode<KeyNodePointerPair>::Get(NodeType::InnerType, nullptr, nullptr, inner_node_max_size_);

            auto smallest_key_splitted_node = splitted_node->Begin()->first;
            // a dummy key for the first slot of the internal node
            KeyNodePointerPair p1 = std::make_pair(smallest_key_splitted_node, node);
            KeyNodePointerPair p2 = std::make_pair(smallest_key_splitted_node, splitted_node);

            auto new_root_node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(root_);
            new_root_node->InsertElementIfPossible(p1, 0);
            new_root_node->InsertElementIfPossible(p2, 1);
            std::cout << "Created new root with children: " << new_root_node->GetCurrentSize() << std::endl;

            return false;
        }

    private:
        BaseNode *root_;
        int inner_node_max_size_;
        int leaf_node_max_size_;
    };

}
#endif //BPLUSTREE_H
