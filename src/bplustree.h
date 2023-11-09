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

        BaseNode *GetSiblingLeft() { return sibling_left_; }

        BaseNode *GetSiblingRight() { return sibling_right_; }

        void SetSiblingLeft(BaseNode *node) { sibling_left_ = node; }

        void SetSiblingRight(BaseNode *node) { sibling_right_ = node; }

        KeyNodePointerPair GetLowKeyPair() { return low_key_; }

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
        const KeyNodePointerPair low_key_;

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
         * Let N be the maximum key-value elements which can be stored in
         * the leaf node. In this implementation after inserting (N-1)
         * key-value elements, we split the leaf node.
         *
         * @return true if leaf should split, false otherwise
         */
        bool WillSplitAfterInsert() {
            return this->GetCurrentSize() >= (this->GetMaxSize() - 1);
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
                    current_node = std::prev(iter)->second;
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

            // Insert will be complete here, if the leaf node has space
            if (!(static_cast<LeafNode *>(node)->WillSplitAfterInsert())) {
                if (node->InsertElementIfPossible(element, iter)) {
                    return true;
                }
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

            // Fix sibling pointers to maintain a bidirectional chain
            // of leaf nodes
            split_node->SetSiblingLeft(node);
            split_node->SetSiblingRight(node->GetSiblingRight());
            node->SetSiblingRight(split_node);

            bool insertion_finished = false;
            BaseNode *current_split_node = split_node;
            auto partition_key = split_node->Begin()->first;

            while (!insertion_finished && !node_search_path.empty()) {
                auto inner_node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(*node_search_path.rbegin());
                node_search_path.pop_back();

                KeyNodePointerPair inner_node_element = std::make_pair(partition_key, current_split_node);

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

                    current_split_node = split_inner_node;
                }
            }

            // If insertion has not finished by now the split propagated
            // back up to the root. So we now need to split the root node
            // as well
            if (!insertion_finished) {
                auto old_root = root_;

                auto smallest_key = current_split_node->GetType() == NodeType::InnerType ?
                                    std::next(static_cast<InnerNode *>(current_split_node)->Begin())->first :
                                    static_cast<LeafNode *>(current_split_node)->Begin()->first;
                KeyNodePointerPair root_element = std::make_pair(smallest_key, current_split_node);

                KeyNodePointerPair low_key = std::make_pair(smallest_key, old_root);
                root_ = ElasticNode<KeyNodePointerPair>::Get(NodeType::InnerType, low_key, inner_node_max_size_);

                auto new_root_node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(root_);
                new_root_node->InsertElementIfPossible(
                        root_element, static_cast<InnerNode *>(new_root_node)->FindLocation(root_element.first)
                );
            }

            return true;
        }

    private:
        BaseNode *root_;
        int inner_node_max_size_;
        int leaf_node_max_size_;
    };

}
#endif //BPLUSTREE_H
