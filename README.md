## Structure of a B+Tree
The B+Tree index is a balanced tree. Every path from the root to any 
leaf is the same height.

## Internal Node
* The internal nodes do not store any data. 
* It stores ordered `n-1` keys and `n` child pointers to tree nodes.
* The key lookup method should adjust for the number of pointers not 
  being equal to the number of keys.
* The number of child pointers is known as the _fanout_ of the node.
* At any time may hold a max of `n` child pointers and must hold at 
  least `⌈n/2⌉` or `ceil(n/2)` pointers.
* At any time an internal page is at least half full.
* A child pointer P<sub>i</sub> points to a subtree where the following 
  invariant holds true:
  * K<sub>i - 1</sub> &lt;= search key < K<sub>i</sub>

### Leaf Node
* It stores ordered `n` keys and `n` value entries.
* Leaf nodes can contain as few as `⌈n/2⌉` or `ceil(n/2)` key-value 
  entries.
* Efficient forward and reverse scans are possible by storing 
  sibling pointers in each leaf node.
