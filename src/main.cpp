#include <iostream>
#include "bplustree.h"

using bplustree::BaseNode;
using bplustree::NodeType;

int main() {
    auto node = BaseNode(NodeType::InnerType, 100);
    std::cout << static_cast<int>(node.GetType()) << std::endl;
    return 0;
}