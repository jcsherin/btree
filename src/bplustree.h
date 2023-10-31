#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <utility>

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
        ElasticNode(NodeType p_type, int p_max_size) :
                BaseNode(p_type, p_max_size),
                current_size_{0} {}

        /**
         * Static helper to allocate storage for an elastic node.
         */
        static ElasticNode *Get(NodeType p_type, int p_max_size) {
            auto *alloc = new char[sizeof(ElasticNode) + p_max_size * sizeof(ElementType)];

            /**
             * Construct elastic node in allocated storage
             * https://en.cppreference.com/w/cpp/language/new#Placement_new
             */
            auto elastic_node = reinterpret_cast<ElasticNode *>(alloc);
            new(elastic_node) ElasticNode(p_type, p_max_size);

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

        bool InsertElementIfPossible(NodeType node_type, ElementType element, const int index) {
            /**
             * TODO: Avoid having to check node type here
             */
            switch (node_type) {
                case NodeType::LeafType:
                    // Each leaf node can contain at most N-1 values
                    if (GetCurrentSize() >= GetMaxSize() - 1) return false;
                    break;
                case NodeType::InnerType:
                    return false;
                    break;
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

    private:
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

        int FindLocation(const int key) {
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

            return this->GetOffset(iter);
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
            if (root_ == nullptr) {
                // Create an empty leaf node, which is also the root
                root_ = ElasticNode<KeyValuePair>::Get(NodeType::LeafType, leaf_node_max_size_);
            }

            BaseNode *current_node = root_;
            auto key = element.first;

            // Traversing down the tree to find the leaf node where the
            // key-value element can be inserted
            while (current_node->GetType() != NodeType::LeafType) {
                auto node = reinterpret_cast<InnerNode *>(current_node);

                auto node_index = node->FindLocation(key);
                auto guide_node = node->At(node_index);
                if (key < guide_node.first) {
                    // The most common case where we traverse down the left
                    // node pointer.
                    current_node = node->At(node_index - 1).second;
                } else {
                    // Traversing down the right node pointer. This will
                    // only happen when the key of the element to be inserted
                    // compares greater than the key at the rightmost end
                    // of the current inner node.
                    current_node = guide_node.second;
                }
            }

            auto node = reinterpret_cast<LeafNode *>(current_node);
            auto index_greater_key_leaf = node->FindLocation(key);
            if (node->InsertElementIfPossible(NodeType::LeafType, element, index_greater_key_leaf)) {
                return true;
            }

            // Leaf node has to split
            return false;
        }

    private:
        BaseNode *root_;
        int inner_node_max_size_;
        int leaf_node_max_size_;
    };

}
#endif //BPLUSTREE_H
