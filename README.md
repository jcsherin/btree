# B+Tree

This is a thread-safe implementation of the B+Tree index data structure.
- It implements the Bayer-Schkolnick concurrency protocol described in [Concurrency of Operations on B-Trees](https://pages.cs.wisc.edu/~david/courses/cs758/Fall2007/papers/Concurrency%20of%20Operations.pdf).
- The algorithm for insertion and deletion is from the textbook [Database System Concepts - 7th edition](https://db-book.com/).
- This code is adapted from the B+Tree implementation in the now archived [NoisePage DBMS](https://db.cs.cmu.edu/projects/noisepage/), which can be found [here](https://github.com/cmu-db/noisepage/blob/master/src/include/storage/index/bplustree.h).

TODO
- [ ] B+Tree primer
    - [ ] Why?
    - [ ] What? Structure & Properties
    - [ ] How? Search & Traversal
    - [ ] How? Insertion
    - [ ] How? Deletion
    - [ ] How? Iterators
- [ ] Naive approach to concurrency
- [ ] Goetz Graefe latch vs lock terminology
- [ ] Latch crabbing concurrency
- [ ] Iterator concurrency

### Build

```
mkdir build
cd build

cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

cmake --build . --target <name> // builds only the named targets
```

### Tests
Run build before this.

```
./test/btree_test
```