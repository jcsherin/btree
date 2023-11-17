Todos:

- [x] Insert key-value
    - [x] Root/leaf
    - [x] Disallow duplicate key insertion in leaf
    - [x] Insert up to leaf capacity
    - [x] Split nodes
        - [x] Leaf
        - [x] Inner
            - [x] Special case: first key-value element
        - [x] Root
- [ ] Insert Crab latching (optimistic)
- [ ] Insert Crab latching (pessimistic)
- [ ] Insert tests
    - [ ] Simple Inserts
    - [ ] Leaf Node Splits (single split)
    - [ ] Inner Node Splits (multiple splits)
    - [ ] Multiple splits also involving root node
    - [ ] Structural integrity
- [x] Find key (point query)
- [ ] Find first leaf node
- [ ] Find last leaf node
- [ ] Find key within bounds (range query)
- [ ] Delete key-value
- [ ] Iterators
    - [ ] Begin
    - [ ] Begin(key)
    - [ ] RBegin
    - [ ] RBegin(key)
    - [ ] End
    - [ ] REnd
- [x] Forward Iterator
- [x] Reverse Iterator
- [ ] Destructor - free all node storage
  called
- [ ] Add macros
    - [x] Always assert
    - [ ] Logger
- [ ] Fix: show actual leaf chain when rendering graph

### Known Issues

* Reverse iterator skipping leaf nodes
    * The left sibling pointer was assigned incorrectly
    * Graphviz was not showing actual leaf chain structure
    * Possible source of the issue:
        * During insert where the leaf node splits the left sibling is
          not being assigned correctly
    * Root cause:
        * When a leaf node splits, the newly created split node is
          added to the rhs of the existing leaf node.
        * And when this leaf node which split has an existing rhs
          sibling leaf node, that sibling node's left link will be
          pointing to this original leaf node.
        * But after the split the rhs sibling should now point to the
          split node on the lhs and not the original leaf node.

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