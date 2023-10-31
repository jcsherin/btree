Todos:
- [ ] Insert key-value
  - [x] Root/leaf
  - [x] Disallow duplicate key insertion in leaf
  - [x] Insert up to leaf capacity
  - [ ] Split nodes
    - [ ] Leaf
    - [ ] Inner
    - [ ] Root
  - [ ] Crab latching (optimistic)
  - [ ] Crab latching (pessimistic)
  - [ ] Insert tests
- [ ] Find key (point query)
- [ ] Find key within bounds (range query)
- [ ] Delete key-value
- [ ] Forward Iterator
- [ ] Reverse Iterator
- [ ] Guard for ensuring `ElasticNode::FreeElasticNode` always gets 
  called
- [ ] Add macros
  - [x] Always assert
  - [ ] Logger

Notes:
1. Condition for node splits?
    1. A leaf node must split when there is no more space to insert one 
       more key-value pair. There are two ways to go about it:
       1. Do nothing after the leaf node becomes full after inserting 
          the key-value pair. When the next key-value pair arrives for 
          insertion into this node, a split is performed.
       2. Pre-emptively split the leaf node when it reaches max size 
          after insertion of a key-value pair.
    2. The same choice exists for internal nodes. Perform the split 
       before or after reaching the max size of the node on inserting a 
       key-pointer pair.

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