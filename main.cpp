#include <iostream>

enum class NodeType : int {
    InternalType = 0, LeafType = 1
};

int main() {
    std::cout << "Internal Node: " << static_cast<int>(NodeType::InternalType) << std::endl;
    std::cout << "Leaf Node: " << static_cast<int>(NodeType::LeafType) << std::endl;
    return 0;
}
