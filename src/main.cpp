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
//    auto index = bplustree::BPlusTree(3, 3);
//    index.Insert(std::make_pair(1, 1));
//    index.Insert(std::make_pair(3, 3));
//    index.Insert(std::make_pair(5, 5));
//    index.Insert(std::make_pair(7, 7));
//    index.Insert(std::make_pair(9, 9));
//    index.Insert(std::make_pair(11, 11));
//    index.Insert(std::make_pair(13, 13));

//    index.Insert(std::make_pair(6, 6));
//    index.Insert(std::make_pair(4, 4));
//    index.Insert(std::make_pair(2, 2));
//    std::vector v{3, 6, 9, 12};
//    for (auto &x: v) {
//        index.Insert(std::make_pair(x, x));
//    }
//    std::cout << index.ToGraph() << std::endl;
//    std::cout << "---------" << std::endl;
//
//    std::vector d{24, 33, 18, 21};
    /**
     * Bug:
     * Insert: {3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33}
     * Delete: {3, 15, 12}
     *
     *
     */
    // borrow one previous inner node
//    std::vector d{39, 45, 48};
// merge with previous inner node
//    std::vector d{39, 45, 48, 33, 36};
//    std::cout << index.ToGraph() << std::endl;
//    std::cout << "---------" << std::endl;
//
//    index.Delete(std::make_pair(3, 3));
//    index.Delete(std::make_pair(8, 4));
//    index.Delete(std::make_pair(7, 4));
//
//    index.Delete(std::make_pair(1, 1));

//    std::vector d{9};
//    for (auto &y: d) {
//        index.Delete(std::make_pair(y, y));
//    }
//
//    std::cout << index.ToGraph() << std::endl;
//
//    for (auto iter = index.Begin(); iter != index.End(); ++iter) {
//        std::cout << (*iter).first << std::endl;
//    }

//    std::vector<int> xs(32);
//    std::iota(xs.begin(), xs.end(), 0);
//
//    std::random_device rd;
//    std::mt19937 g(rd());
//
//    std::shuffle(xs.begin(), xs.end(), g);
//    for (auto &x: xs) {
//        std::cout << x << ", ";
//    }
//    std::cout << std::endl;

//    auto index = bplustree::BPlusTree(4, 4);
//
//    std::vector<int> v(16);
//    std::iota(v.begin(), v.end(), 0);
//
//    std::random_device rd;
//    std::mt19937 g(rd());
//
//    std::shuffle(v.begin(), v.end(), g);
//    for (auto &num: v) {
//        index.Insert(std::make_pair(num, num));
//    }
//
//    for (auto iter = index.Begin(); iter != index.End(); ++iter) {
//        std::cout << "Key: " << (*iter).first << " Value: " << (*iter).second << std::endl;
//    }
//
//    for (auto iter = index.RBegin(); iter != index.REnd(); --iter) {
//        std::cout << "Key: " << (*iter).first << " Value: " << (*iter).second << std::endl;
//    }

    bplustree::BPlusTree index{3, 3};

//    std::vector<int> items(1024);
//    std::iota(items.begin(), items.end(), 0);
//
//    std::random_device rd;
//    std::mt19937 g(rd());
//
//    std::shuffle(items.begin(), items.end(), g);

//    std::vector items{18, 5, 4, 3, 6, 13, 10, 1, 14, 0, 20, 8, 19, 22, 16, 15, 12, 2, 21};
//    std::vector<int> items(22);
//    std::iota(items.begin(), items.end(), 0);
//    std::vector items{28, 9, 21, 0, 29, 10, 25, 7, 26, 31, 11, 18, 19, 5, 2, 1, 23, 8, 13, 14, 6, 30, 3, 4, 20, 24, 12, 22, 16, 15, 17, 27};

    std::vector items{3, 6, 9, 12, 15, 18, 21, 24, 4, 5, 7, 8, 10};

    for (auto &i: items) {
        index.Insert(std::make_pair(i, i));
    }
    index.Delete(21);
//    index.Delete(std::make_pair(9, 9));

    std::cout << index.ToGraph() << std::endl;

    return 0;
}