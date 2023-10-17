Todos:
- [ ] Insert key-value
- [ ] Setup tests
- [ ] Find key (point query)
- [ ] Find key within bounds (range query)
- [ ] Delete key-value
- [ ] Crab latching (optimistic) protocol
- [ ] Forward Iterator
- [ ] Reverse Iterator
- [ ] Guard for ensuring `ElasticNode::FreeElasticNode` always gets 
  called
- [ ] Add an always assert macro for debugging


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
