#include <iostream>

enum class NodeType : int {
    InternalNode = 0, LeafNode = 1
};

class NodeHeader {
public:
    /** The type of the node */
    NodeType node_type_;

    /** Count of key-value pairs in this node */
    int item_count_;

    /**
     * Constructor
     */
    NodeHeader(NodeType node_type, int item_count) : node_type_{node_type}, item_count_{item_count} {}
};

class BaseNode {
public:
    /**
     * Constructor - Initialize header
     */
    BaseNode(NodeType node_type, int item_count) : header_{node_type, item_count} {}

    /**
     *
     * @return The type of the node
     */
    NodeType GetType() const { return header_.node_type_; }

    /**
     *
     * @return The count of key-value entries in this node
     */
    int GetSize() const { return header_.item_count_; }

    /**
     *
     * @return A const reference to the header of the node
     */
    const NodeHeader &GetNodeHeader() const { return header_; }

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
    /**
     * The header contains:
     * 1. type of the node
     * 2. count of key-value entries
     */
    NodeHeader header_;
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
    ElasticNode(int size, NodeType node_type, int item_count) :
            BaseNode(node_type, item_count), size_{size} {}

    /**
     *
     * @return The length of the `data_` array
     */
    int GetSize() const { return size_; }

    static ElasticNode *Get(int size, NodeType node_type, int item_count) {
        auto *alloc_base = new char[sizeof(ElasticNode) + size * sizeof(ElementType)];

        auto elastic_node = reinterpret_cast<ElasticNode *>(alloc_base);
        /**
         * Placement new: See https://en.cppreference.com/w/cpp/language/new#Placement_new
         */
        new(elastic_node) ElasticNode{size, node_type, item_count};

        return elastic_node;
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

int main() {
    auto inner = BaseNode(NodeType::InternalNode, 10);
    auto leaf = BaseNode(NodeType::LeafNode, 4);

    std::cout << std::boolalpha
              << "Is internal node? " << inner.IsInternalNode()
              << std::noboolalpha
              << " size: " << inner.GetSize()
              << std::endl;

    std::cout << std::boolalpha
              << "Is leaf node? " << leaf.IsLeafNode()
              << std::noboolalpha
              << " size: " << leaf.GetSize()
              << std::endl;

    auto tree = BPlusTreeBase();
    std::cout << "Internal max: " << tree.GetInternalNodeMaxSize()
              << " min: " << tree.GetInternalNodeMinSize()
              << std::endl;
    std::cout << "Leaf max: " << tree.GetLeafNodeMaxSize()
              << " min: " << tree.GetLeafNodeMinSize()
              << std::endl;

    return 0;
}
