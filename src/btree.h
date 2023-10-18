
#ifndef BTREE_BTREE_H
#define BTREE_BTREE_H

#include "macros.h"

enum class NodeType : int {
    InternalNode = 0, LeafNode = 1
};

class BaseNode {
public:
    /**
     * Constructor - Initialize header
     */
    BaseNode(NodeType node_type) : node_type_{node_type} {}

    /**
     *
     * @return The type of the node
     */
    NodeType GetType() const { return node_type_; }

    /**
     *
     * @return true if the node is a leaf node
     */
    bool IsLeafNode() const { return GetType() == NodeType::LeafNode; }

    /**
     *
     * @return true if the node is an internal node
     */
    bool IsInternalNode() const { return GetType() == NodeType::InternalNode; }

private:
    /** The type of the node */
    NodeType node_type_;
};

/*
 * The base class of elastic node types - `InternalNode` & `LeafNode`
 */
template<typename ElementType>
class ElasticNode : public BaseNode {
private:
    /**
     * The length of the `data_` array
     */
    int size_;

    /**
     * Compiler voodoo (struct hack aka flexible array member):
     * https://developers.redhat.com/articles/2022/09/29/benefits-limitations-flexible-array-members
     *
     * The data array will be allocated separately
     */
    ElementType data_[0];

public:
    /**
     * Constructor
     */
    ElasticNode(int size, NodeType node_type) : BaseNode(node_type), size_{size} {}

    /**
     *
     * @return The length of the `data_` array
     */
    int GetSize() const { return size_; }

    /**
     * Call this static method to construct an elastic node of a certain size.
     *
     * @param size the fanout of the node
     * @param node_type which is either `InternalNode` or  `LeafNode`
     * @return
     */
    static ElasticNode *Get(int size, NodeType node_type) {
        auto *alloc_base = new char[sizeof(ElasticNode) + size * sizeof(ElementType)];

        auto elastic_node = reinterpret_cast<ElasticNode *>(alloc_base);
        /**
         * Placement new: See https://en.cppreference.com/w/cpp/language/new#Placement_new
         */
        new(elastic_node) ElasticNode{size, node_type};

        return elastic_node;
    }

    /**
     * The struct hack makes it possible to have an embedded data array of
     * dynamic size while keeping a flat memory layout. In the constructor
     * the data array starts out empty, and the space for it is allocated
     * only after we call the static `ElasticNode::Get` method.
     *
     * In `Get` placement new is used to separate allocation and construction
     * of the elastic node. The destructor is not automatically called on
     * such a node, and we therefore have to call the `FreeElasticNode`
     * method to free the memory we allocated for the data array.
     */
    void FreeElasticNode() {
        ElasticNode *allocation_start = this;
        delete[] reinterpret_cast<char *>(allocation_start);
    }

};

class BPlusTreeBase {
public:
    /**
     * Constructor
     */
    BPlusTreeBase() = default;

    /**
     * Destructor
     */
    ~BPlusTreeBase() = default;

    /**
     *
     * @return the size for internal node split [fanout]
     */
    int GetInternalNodeMaxSize() const { return internal_node_max_size_; }

    /**
     *
     * @return the size for internal node removal [⌈fanout / 2⌉ - 1]
     */
    int GetInternalNodeMinSize() const { return internal_node_min_size_; }

    /**
     *
     * @return the size for leaf node split [fanout]
     */
    int GetLeafNodeMaxSize() const { return leaf_node_max_size_; }

    /**
     *
     * @return the size for leaf node removal [⌈(fanout - 1) / 2⌉]
     */
    int GetLeafNodeMinSize() const { return leaf_node_min_size_; }

private:
    int internal_node_max_size_ = 128;
    int internal_node_min_size_ = 63; /** ⌈fanout / 2⌉ - 1 */

    int leaf_node_max_size_ = 128;
    int leaf_node_min_size_ = 64; /** ⌈(fanout - 1) / 2⌉ */
};

class BPlusTree : public BPlusTreeBase {
public:
    /**
     * Constructor
     */
    explicit BPlusTree() : BPlusTreeBase(), root_{nullptr}, num_keys_{0} {}

    /**
     * Destructor
     */
    ~BPlusTree() {}

    /**
     *
     * @param key
     * @param value
     * @return true on successful insertion, false otherwise
     *
     * Support only for unique keys. If the key already exists in the index
     * the insertion will not be performed and will return false.
     */
    auto Insert(int key, int value) -> bool {
        BTREE_ASSERT(true, "testing always assertion");
        return false;
    }

private:
    BaseNode *root_;
    uint64_t num_keys_;
};

#endif //BTREE_BTREE_H
