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
- [x] Find key (point query)
- [x] Find first leaf node
- [x] Find last leaf node
- [ ] Insert Crab latching (optimistic)
- [ ] Iterators
    - [x] Begin
    - [ ] Begin(key)
    - [x] RBegin
    - [ ] RBegin(key)
    - [x] End
    - [x] REnd
- [x] Forward Iterator
- [x] Reverse Iterator
- [x] Destructor - free all tree nodes
- [ ] Add macros
    - [x] Always assert
    - [ ] Logger
- [ ] Insert Crab latching (pessimistic)
- [ ] Insert tests
    - [ ] Simple Inserts
    - [ ] Leaf Node Splits (single split)
    - [ ] Inner Node Splits (multiple splits)
    - [ ] Multiple splits also involving root node
    - [ ] Structural integrity
- [ ] Find key within bounds (range query)
- [ ] Delete key-value
- [ ] Fix: show actual leaf chain when rendering graph
- [ ] Change parameter of `Delete` to key 

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