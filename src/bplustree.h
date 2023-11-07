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
            ElasticNode *new_node = this->Get(this->GetType(), nullptr, nullptr, this->GetMaxSize());
            ElementType *copy_from_location = std::next(this->Begin(), FastCeilIntDivision(this->GetCurrentSize(), 2));


            auto copied_count = std::distance(copy_from_location, End());
            std::memcpy(
                    reinterpret_cast<void *>(new_node->Begin()),
                    reinterpret_cast<void *>(copy_from_location),
                    copied_count * sizeof(ElementType)
            );
            new_node->SetCurrentSize(copied_count);
            this->SetCurrentSize(std::distance(this->Begin(), copy_from_location));

            return new_node;
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

        ElementType *GetElementAt(ElementType *location) { return location; }

        int GetCurrentSize() { return current_size_; }

        void SetCurrentSize(int value) { current_size_ = value; }

        bool InsertElementIfPossible(const ElementType &element, ElementType *location) {
            if (GetCurrentSize() >= GetMaxSize()) {
                return false;
            }

            if (std::distance(location, End()) > 0) {
                std::memmove(
                        reinterpret_cast<void *>(std::next(location)),
                        reinterpret_cast<void *>(location),
                        std::distance(location, End()) * sizeof(ElementType)
                );
            }

            new(location) ElementType{element};
            ++current_size_;

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

        KeyValuePair *FindLocation(const int key) {
            KeyValuePair dummy = std::make_pair(key, 0);
            KeyValuePair *iter = std::lower_bound(this->Begin(), this->End(), dummy,
                                                  [](const auto &a, const auto &b) {
                                                      return a < b;
                                                  });
            return iter;
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
                root_ = ElasticNode<KeyValuePair>::Get(NodeType::LeafType, nullptr, nullptr, leaf_node_max_size_);
            }

            BaseNode *current_node = root_;
            std::vector<BaseNode *> node_search_path{}; // stack of node pointers

            // Traversing down the tree to find the leaf node where the
            // key-value element can be inserted
            while (current_node->GetType() != NodeType::LeafType) {
                auto node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(current_node);
                node_search_path.push_back(current_node);

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
            auto iter = static_cast<LeafNode *>(node)->FindLocation(element.first);
            if (iter != node->End() && element.first == iter->first) {
                return false;   // duplicate key
            }

            BPLUSTREE_ASSERT(element.first != iter->first, "Inserting unique key");
            // If the leaf node will become full after inserting the current
            // key-value element then we split the leaf node
            auto leaf_will_split = node->GetCurrentSize() >= (node->GetMaxSize() - 1);
            if (!leaf_will_split && node->InsertElementIfPossible(element, iter)) {
                return true;
            }

            // Split the leaf node
            BPLUSTREE_ASSERT(node->GetCurrentSize() == (node->GetMaxSize() - 1),
                             "node will become full after next insert");


            auto split_node = node->SplitNode();
            if (element.first < split_node->Begin()->first) {
                node->InsertElementIfPossible(
                        element,
                        static_cast<LeafNode *>(node)->FindLocation(element.first)
                );
            } else {
                split_node->InsertElementIfPossible(
                        element,
                        static_cast<LeafNode *>(split_node)->FindLocation(element.first)
                );
            }

            // Insert the newly split leaf node into the chain
            split_node->SetSiblingLeft(node);
            split_node->SetSiblingRight(node->GetSiblingRight());
            node->SetSiblingRight(split_node);

            bool insertion_finished = false;
            BaseNode *new_child_node = split_node;
            while (!insertion_finished && !node_search_path.empty()) {
                auto parent = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(*node_search_path.rbegin());
                node_search_path.pop_back();

                auto smallest_key = (new_child_node->GetType() == NodeType::InnerType) ?
                                    std::next(static_cast<InnerNode *>(new_child_node)->Begin())->first :
                                    static_cast<LeafNode *>(new_child_node)->Begin()->first;
                KeyNodePointerPair inner_node_element = std::make_pair(smallest_key, new_child_node);

                if (parent->InsertElementIfPossible(
                        inner_node_element,
                        static_cast<InnerNode *>(parent)->FindLocation(inner_node_element.first)
                )) {
                    // we are done if insertion is possible without further splitting
                    insertion_finished = true;
                    break;
                }

                // Recursively split the node
                auto split_parent = parent->SplitNode();
                if (inner_node_element.first < split_parent->Begin()->first) {
                    parent->InsertElementIfPossible(inner_node_element,
                                                    static_cast<InnerNode *>(parent)->FindLocation(
                                                            inner_node_element.first));
                } else {
                    split_parent->InsertElementIfPossible(inner_node_element,
                                                          static_cast<InnerNode *>(split_parent)->FindLocation(
                                                                  inner_node_element.first));
                }

                new_child_node = split_parent;
            }

            // If insertion has not finished by now the split propagated
            // back up to the root. So we now need to split the root node
            // as well
            if (!insertion_finished) {
                auto old_root = root_;

                auto smallest_key = new_child_node->GetType() == NodeType::InnerType ?
                                    std::next(static_cast<InnerNode *>(new_child_node)->Begin())->first :
                                    static_cast<LeafNode *>(new_child_node)->Begin()->first;
                KeyNodePointerPair root_element = std::make_pair(smallest_key, new_child_node);

                root_ = ElasticNode<KeyNodePointerPair>::Get(NodeType::InnerType, nullptr, nullptr,
                                                             inner_node_max_size_);
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
