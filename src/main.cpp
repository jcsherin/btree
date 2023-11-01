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

    int fanout_even = 8;
    int fanout_even_max = fanout_even - 1;

    int fanout_odd = 9;
    int fanout_odd_max = fanout_odd - 1;

    std::cout << "fanout: " << fanout_even
              << " max: " << fanout_even_max
              << " min: " << fanout_even_max / 2
              << " ceil(min): " << (fanout_even_max + 2 - 1) / 2
              << " ceil(min no-overflow): " << 1 + ((fanout_even_max - 1) / 2)
              << std::endl;

    std::cout << "fanout: " << fanout_odd
              << " max: " << fanout_odd_max
              << " min: " << fanout_odd_max / 2
              << " ceil(min): " << (fanout_odd_max + 2 - 1) / 2
              << " ceil(min no-overflow): " << 1 + ((fanout_odd_max - 1) / 2)
              << std::endl;
    return 0;
}