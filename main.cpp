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
     * Constructor
     */
    BaseNode() = default;

    /**
     * Destructor
     */
    ~BaseNode() = default;

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
};

int main() {
    auto b = BaseNode();

    std::cout << "Internal node max: " << b.GetInternalNodeMaxSize() << " min: " << b.GetInternalNodeMinSize()
              << std::endl;
    std::cout << "Leaf node max: " << b.GetLeafNodeMaxSize() << " min: " << b.GetLeafNodeMinSize() << std::endl;

    auto internal_header = NodeHeader(NodeType::InternalNode, 5);
    auto leaf_header = NodeHeader(NodeType::LeafNode, 4);

    std::cout << "Node type: " << static_cast<int>(internal_header.node_type_) << " size: " << internal_header.size_
              << std::endl;
    std::cout << "Node type: " << static_cast<int>(leaf_header.node_type_) << " size: " << leaf_header.size_
              << std::endl;

    return 0;
}
