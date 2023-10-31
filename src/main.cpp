#include <iostream>
#include "bplustree.h"

int main() {
    auto tree = bplustree::BPlusTree(4, 4);
    tree.Insert(std::make_pair(150, 456));
    tree.Insert(std::make_pair(123, 456));
    tree.Insert(std::make_pair(100, 456));
    // try duplicate insertion
    tree.Insert(std::make_pair(150, 456));
    tree.Insert(std::make_pair(123, 456));
    tree.Insert(std::make_pair(100, 456));
    // insert one more
    tree.Insert(std::make_pair(110, 456));
    // from here on nothing will be inserted
    tree.Insert(std::make_pair(130, 456));
    tree.Insert(std::make_pair(140, 456));
    tree.Insert(std::make_pair(135, 456));

    return 0;
}