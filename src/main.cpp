#include <iostream>
#include "btree.h"

int main() {
    auto elastic_node = ElasticNode<std::pair<int, int>>::Get(10, NodeType::LeafNode);
    elastic_node->FreeElasticNode();

    return 0;
}
