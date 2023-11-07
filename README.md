Todos:

- [ ] Insert key-value
    - [x] Root/leaf
    - [x] Disallow duplicate key insertion in leaf
    - [x] Insert up to leaf capacity
    - [x] Split nodes
        - [x] Leaf
        - [ ] Inner
            - [ ] Special case: first key-value element
        - [x] Root
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