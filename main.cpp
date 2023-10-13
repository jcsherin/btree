#include <iostream>
#include <cmath>

enum class NodeType : int {
    InternalNode = 0, LeafNode = 1
};

class NodeHeader {
public:
    /** The type of the node */
    NodeType node_type_;

    /** Count of key-value pairs in this node */
    int size_;

    /**
     * Constructor
     */
    NodeHeader(NodeType node_type, int size) : node_type_{node_type}, size_{size} {}
};

class BaseNode {
public:
    /**
     * Constructor - Initialize header
     */
    BaseNode(NodeType node_type, int size) : header_{node_type, size} {}

    /**
     *
     * @return The type of the node
     */
    NodeType GetType() const { return header_.node_type_; }

    /**
     *
     * @return The count of key-value entries in this node
     */
    int GetSize() const { return header_.size_; }

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
    int internal_node_min_size_ = std::ceil(internal_node_max_size_ / 2) - 1;

    int leaf_node_max_size_ = 128;
    int leaf_node_min_size_ = std::ceil((leaf_node_max_size_ - 1) / 2);

    NodeHeader header_;
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

    return 0;
}
