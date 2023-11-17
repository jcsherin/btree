#include <iostream>
#include "bplustree.h"
#include <numeric>
#include <random>

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
    auto index = bplustree::BPlusTree(4, 4);

    std::vector<int> v(16);
    std::iota(v.begin(), v.end(), 0);

    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(v.begin(), v.end(), g);
    for (auto &num: v) {
        index.Insert(std::make_pair(num, num));
    }

    for (auto iter = index.Begin(); iter != index.End(); ++iter) {
        std::cout << "Key: " << (*iter).first << " Value: " << (*iter).second << std::endl;
    }

    for (auto iter = index.RBegin(); iter != index.REnd(); --iter) {
        std::cout << "Key: " << (*iter).first << " Value: " << (*iter).second << std::endl;
    }

    return 0;
}