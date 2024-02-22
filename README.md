# B+Tree

This is a thread-safe implementation of the B+Tree index data structure.

- It implements the Bayer-Schkolnick concurrency protocol described
  in [Concurrency of Operations on B-Trees](https://pages.cs.wisc.edu/~david/courses/cs758/Fall2007/papers/Concurrency%20of%20Operations.pdf).
- The algorithm for insertion and deletion is from the
  textbook [Database System Concepts - 7th edition](https://db-book.com/).
- This code is adapted from the B+Tree implementation in the now
  archived [NoisePage DBMS](https://db.cs.cmu.edu/projects/noisepage/),
  which can be
  found [here](https://github.com/cmu-db/noisepage/blob/master/src/include/storage/index/bplustree.h).

## Draft
The B+Tree is an index data structure commonly found in transactional
databases, and filesystems. It supports efficient point queries, range
queries and iterating over keys in sorted order without the need for an
explicit sorting step. The search is efficient because the B+Tree is a
balanced tree data structure meaning, the height of the tree is same no
matter which path you take from the root to any of the leaf nodes. The
data is stored only in the leaf nodes and there is a bidirectional link
between sibling leaf nodes. The purpose of inner nodes is help with
traversals from root to the correct leaf node for key lookups. Even
though key lookups are trivial, it is challenging to maintain the
balance of a B+Tree when keys are being inserted or deleted. An
implementation is made even more challenging when it has to support
concurrent readers and writers.

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