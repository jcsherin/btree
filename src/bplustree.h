#ifndef BPLUSTREE_H
#define BPLUSTREE_H

namespace bplustree {
    enum class NodeType : int {
        InnerType = 0, LeafType = 1
    };

    class Node {
    public:
        Node(NodeType p_type, int p_max_size) :
                type_{p_type},
                max_size_{p_max_size},
                curr_size_{0} {}

        NodeType GetType() { return type_; }

    private:
        NodeType type_;
        int max_size_;
        int curr_size_;
    };
}
#endif //BPLUSTREE_H
