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
A B+Tree is an index data structure with many variants and of widespread
use in transactional database systems. It supports efficient:

- point queries (key has to be an exact match)
- range queries (keys within a range which satisfies a predicate)
- forward and reverse sequential scans (in sorted key order)
  The B+Tree is a balanced tree data structure. It means that the
  distance from the root node to the leaf node is the same regardless of
  which path is traversed. The key-value data pairs are stored only at
  the leaf node level. A new key-value pair is always inserted in sorted
  order in a leaf node. The internal nodes exist to guide key based
  search and traversing to the right leaf node where a key maybe found.

The leaf nodes are also connected to their siblings like a doubly linked
list which makes range queries, and sequential scans possible. The
database optimiser can generate a query plan which avoids an explicit
sort step for queries which specify a sort order on a column attribute
if there is a B+Tree index defined on that column. This is possible
because the keys are already in sorted order, and possible to scan the
leaf nodes sequentially in both forward and reverse directions.

The balanced natured of the B+Tree also guarantees stable performance
for key lookups regardless of the total number of keys. The lookup time
depends only on the total number of keys stored in the B+Tree and the
node degree. Node degree is the total child nodes a node can have. It
also alternatively known as branching factor or fanout. For example, a
B+Tree with a billion key-values and node degree of 64 has to search no
more than 5 nodes in the entire tree to find a match.

Nearly 99% of the nodes in a B+Tree are leaf nodes and only 1% of the
nodes are internal nodes. So even when the B+Tree stores a large number
of key-values and they do not fit into memory it maybe possible to fit
the B+Tree into memory all the internal nodes. The leaf node being
searched is then just one disk I/O away. Hot leaf nodes can be cached
using a limited amount of additional memory, and a cache eviction policy
which is resistant to sequential flooding, further reducing disk I/O for
search queries.

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