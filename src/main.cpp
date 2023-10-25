#include <iostream>
#include "btree.h"

int main() {
    auto tree = BPlusTree();
    tree.Insert(1, 1);

    return 0;
}