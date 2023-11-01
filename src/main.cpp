#include <iostream>
#include "bplustree.h"

int FastCeilIntDivision(int x, int y) {
    return 1 + ((x - 1) / y);
}

void print_fanout_row(int fanout, int fanout_max) {
    std::cout << "fanout: " << fanout
              << " max: " << fanout_max
              << " min: " << fanout_max / 2
              << " ceil(min): " << (fanout_max + 2 - 1) / 2
              << " ceil(min no-overflow): " << FastCeilIntDivision(fanout_max, 2)
              << std::endl;
}

void print_fanout() {
    int i = 2;
    while (i <= 1024) {
        print_fanout_row(i, i - 1); // even
        print_fanout_row(i + 1, i + 1 - 1); // odd

        if (i > 2) {
            i *= 2;
        } else {
            i = 8;
        }
    }
}

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