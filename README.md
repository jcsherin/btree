[![C++ CI](https://github.com/jcsherin/btree/actions/workflows/ci.yml/badge.svg)](https://github.com/jcsherin/btree/actions/workflows/ci.yml)

## B+Tree

Here you’ll find a thread-safe, in-memory implementation of the B+Tree
data structure. I wrote this to learn first-hand the challenges involved
in implementing a concurrent data structure correctly.

- The algorithm for key-value lookups, insertion and deletion are taken from the
  textbook — [Database System Concepts - 7th edition](https://db-book.com/).
- The Bayer-Schkolnick concurrency protocol is the one described in the
  paper - [Concurrency of Operations on B-Trees](https://pages.cs.wisc.edu/~david/courses/cs758/Fall2007/papers/Concurrency%20of%20Operations.pdf).
- The interface and data structure layout code is adapted
  from [cmu-db/noisepage: Self-Driving Database Management System from Carnegie Mellon University](https://github.com/cmu-db/noisepage)

### Interface

Create a B+Tree index with an inner node fanout of 31, and a leaf node
fanout of 32.

```c++
auto index = BPlusTree(31, 32);
```

Insert a 100 key-value elements like `(0, 0)`, `(1, 1)`, `(2, 2)` etc.
into the index.

```c++
std::vector<int> keys(100);
std::iota(keys.begin(), keys.end(), 0);

for (auto &key: keys) {
	index.Insert(std::make_pair(key, key));
} 
```

Lookup key-value elements.

```c++
index.MaybeGet(50); 	// 50
index.MaybeGet(100); 	// std::nullopt
```

Delete key-value elements.

```c++
auto deleted_1 = index.Delete(10); 		// deleted_1: true
auto deleted_2 = index.Delete(110); 	// deleted_2: false
```

Forward iteration,

```c++
for (auto iter = index.Begin(); iter != index.End(); ++iter) {
	int key = (*iter).first;
	int value = (*iter).second;
}
```

Reverse iteration,

```c++
for (auto iter = index.RBegin(); iter != index.REnd(); --iter) {
	int key = (*iter).first;
	int value = (*iter).second;
}
```

### Build

1. Create the build directory.

```sh
mkdir -p build
```

2. Run CMake in the build directory.

```sh
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

3. Build tests

```sh
make btree_insert_test
make btree_delete_test
make btree_concurrent_test
```

4. Run the tests

```sh
./test/btree_insert_test
./test/btree_delete_test
./test/btree_concurrent_test
```

### Clean Up

To remove the compiled files from the build directory, you can run the `clean` target that CMake generates for `make`.

```shell
cd build
make clean
```

To remove the entire build directory and all its contents, you can run the following command from the project's root
directory:

```shell
rm -rf build
```

### Testing

The [tests](https://github.com/jcsherin/btree/tree/main/test) uses a
small branching factor to trigger node overflows and underflows
frequently. When combined with a high volume of key-value operations it
helped catch many bugs early in the development of the concurrent B+Tree
data structure.

The [concurrent tests](https://github.com/jcsherin/btree/blob/main/test/btree_concurrent_test.cpp)
uses both key randomization and interleaving of insertion/deletion
operations to surface bugs in the concurrency protocol.

Here are a few bugs revealed by the above tests,

1. [Insertion deadlock](https://github.com/jcsherin/btree/blob/main/src/bplustree.h#L993-L1006)
2. [Split internal node with insufficient node pointers](https://github.com/jcsherin/btree/blob/aa2c6c22f1cc47cd4ea1aa3f98443e5140f6cc05/src/bplustree.h#L1023-L1048)
3. [Recheck conditions after entering critical section](https://github.com/jcsherin/btree/blame/aa2c6c22f1cc47cd4ea1aa3f98443e5140f6cc05/src/bplustree.h#L1191-L1203)
4. [Prevent data-race when updating B+Tree root](https://github.com/jcsherin/btree/blame/aa2c6c22f1cc47cd4ea1aa3f98443e5140f6cc05/src/bplustree.h#L1501-L1517)

### Modified Delete Rebalancing

When optimistic delete fails, the B+Tree is rebalanced by first
attempting to merge with a sibling node, and when that is not possible
by borrowing a key-value element from a sibling node. This
implementation switches the order and choose to borrow a key-value
element first from the sibling node, before trying to merge with a
sibling node.

This has the advantage of completing deletion earlier. When the borrow
is successful, then the deletion is done and the B+Tree is rebalanced.

On the other hand when merging nodes, the parent internal node needs to
be updated and it has a small possibility of underflowing as well. If
that happens then rebalancing has propagated up the B+Tree and the
deletion has to continue with tree rebalancing.

So this implementation changes the order and first attempt to borrow,
and only when that fails proceed with merge.

### Concurrency Protocol

Each node contains a latch which can be latched in shared/exclusive mode
to protect the key-value elements from being simultaneously accessed by
multiple threads. The concurrency protocol ensures that traversals,
lookups, forward/reverse iterations of the B+Tree data structure works
correctly even when interleaved with tree modifying operations like
insertions/deletions.

Several threads can own the shared latch on a node. This is useful for
read-only operations on the node. But only one thread at a time can own
an exclusive latch on a node. This is used for writing to the node.

During traversal of the tree it’s important to ensure that the node
still exists in the tree and has not been removed or merged into another
node by another concurrent operation. So the thread holds the latch for
the parent node, while it tries to acquire a latch for the child node.
Only after the child node latch is acquired is the parent node latch
released. This ensures that no concurrent modifications happened in
between which invalidated the existence of the child node from the tree.
This technique is known as crab-latching.

In the case of forward/reverse iteration the thread attempts to acquire
a latch on the node in a non-blocking manner. If unable to acquire a
latch on the next sibling node all the latches currently held are
released and the iteration itself is terminated.

Deadlocks are avoided through programming discipline. The latches are
acquired only in a single direction when traversing the tree from the
root to the leaf nodes. In the case of forward/reverse iteration all
held latches are released immediately if the next node cannot be
latched. Both of these ensure that there will be no deadlocks.

To have the maximum possible concurrency we have to take advantage of
the fact that not every concurrent operation in the tree will happen in
overlapping nodes. Also for every insertion/deletion if we acquired an
exclusive latch from root to the leaf node, there would always be
contention at the root node and reduce concurrency to being no better
than sequential access. We instead rely on the fact that even when
concurrent insertions/deletions overlap within the same leaf node and
tree path, not all of them will lead to an overflow/underflow of the
modified node. So we can get away will holding exclusive latches in more
limited fashion improving concurrency.

For insertions/deletions we check if a node is safe. A node is safe if
the next insertion/deletion will not lead to node overflow/underflow.
During traversal if the node below in the tree is safe then shared
latches are acquired. All latches we acquired above are released. When a
node is deemed unsafe because it will overflow/underflow then we acquire
an exclusive latch on the node.

To further improve concurrency in the first attempt we optimistically
traverse the tree for insertions/deletions under the assumption that an
exclusive latch will only be needed for the leaf node modification and
this will not result in an overflow/underflow. This is going to be true
most of the times. Only shared latches are acquired from the root to the
leaf node. To modify the leaf node the shared latch is released and an
exclusive latch is acquired. If at this point we realise that node will
overflow/underflow the optimistic approach is abandoned. We restart
traversal from the root of the tree this time acquiring exclusive
latches along the path if there are unsafe nodes.
