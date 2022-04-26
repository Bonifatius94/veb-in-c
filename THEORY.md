
# Theoretical Insight

## About
This document provides information on the van Emde Boas tree
data structure and some common improvements to make it feasible.

## Purpose of Veb Trees
The van Emde Boas tree data structure serves as a **priority queue**
(the term 'tree' is a bit misleading). It supports all typical
priority queue operations in O(log log u) time which is
*almost constant*, e.g. u = 2^64 -> log_2 log_2 2^64 = log_2 64 = 6.

| Operation   | Time Complexity |
| ----------- | --------------- |
| Min         | O(1)            |
| Max         | O(1)            |
| Lookup      | O(log log u)    |
| Successor   | O(log log u)    |
| Predecessor | O(log log u)    |
| Insert      | O(log log u)    |
| Delete      | O(log log u)    |

*Note that these operations are a bit different to binary or Fibonacci 
heaps.*

But as always, there's also a downside. Unfortunately, the van Emde Boas
structure requires lots of memory, up to O(u log u) bits without and
O(u) bits with improvements. For example let u = 2^64, then the memory
consumption would be in exabyte range.

So, allocating a full van Emde Boas tree of u = 2^64 is pretty much
unfeasible even with giant machines. Therefore we'll make some 
improvements to the data structure later. But first, let's dive
into the core ideas of van Emde Boas trees.

## Basic Data Structure
In this chapter, the basic van Emde Boas tree structure gets explained
step-by-step.

### Global / Local Pattern
The core idea of the van Emde Boas data structure is splitting up the
universe into global / local subtrees where the global is bookkeeping
which locals exist and the locals store the payload. As the data 
structure is self-similar, each global / local consists itself of
smaller global / local structures, cascading down to a single bit.

### Data Distribution
Now, using this idea of global / local structures, it's still unclear
how the time complexity of O(log log u) is even achieved. So, there's
basically a trick where the universe bits managed by a tree structure
are halved at each tree layer. This partitioning strategy results in
both global and local structures managing only a universe u' of size
sqrt(u), rounded to the higher / lower power of 2.

Moreover, there are pointers to the smallest / largest key inserted
into each structure, called low / high, such that the min / max lookup
is very efficient in O(1) time if we keep the pointers up-to-date.

### Low Pointer Trick
But this doesn't suffice. The insert() and delete() operations are still
only O(log u). Traversing a single path from the top to the bottom of
the entire tree yields our desired time complexity of O(log log u)
because the tree is only O(log u) layer high,
but there might be cascading insertions / deletions, so an operation
could explore various paths, ruining the performance.

Now, instead of carrying out an amortized analysis for showing that
it's not too bad anyways (like in the Fibonacci heap structure),
Peter van mde Boas came up with another brilliant idea. He excluded the
data stored in the low pointer from the tree. This little trick
guaruntees that there are no cascading insertions / deletions because
the low pointer is not part of the tree and therefore doesn't cause any 
recursive operation calls. That way, insert() and delete() also
yield the time complexity of O(log log u).

### Memory Allocation
Maybe we did agree a bit too fast on the performance of the insert()
operation, though because the allocation of the locals array when 
creating a new tree node needs to be considered as well.
As there are sqrt(u) locals in the tree node, an allocation
would take O(sqrt(u)) time, ruining our performance once again.
So, the van Emde Boas tree needs to be fully allocated upfront
in O(u) time, requiring O(u log u) bits of memory. Of course
those memory hunger needs to be addressed to make the van Emde Boas
tree feasible, which we'll see in the following sections.

## Improvements
Now that the basic ideas are clear, we'll go into common improvements
to the van Emde Boas tree.

### Bitwise Tree Leafs
Managing small universes with the naive approach can become quite
inefficient. For managing universes of size u = 64, this cascades into
33 sub-structures managing 32 keys, each of them once again cascading to
17 sub-structures, and so on. So there's plenty of memory consumption.

Gladly, 64-bit bitboards can serve all tree operations in O(1) time,
so nodes managing less than 64 keys can be replaced by a bitboard leaf.
This reduces the memory consumption already by a lot.

## Memory Efficient Root
Next, we'll make use of the global / local pattern, but adjust the
data distribution between global and local structures such that each
local only manages log_2(u) keys. Moreover, the locals are implemented
as binary search trees / bitboards, still yielding a performance of
O(log log u) because of only managing log_2(u) keys. As a consequence, the global structure manages u / log_2(u) keys (as a van Emde Boas
tree), still providing the desired time complexity of O(log log u).

Combining the memory consumption of global and locals, we see that
there are O(u / log_2(u)) locals requiring O(log_2(u)) bits each and
the global with O((u / log_2 u) log_2 (u / log_2 u)) < O(u) bits.
So both parts of the tree are now within O(u) bits of memory.

*Note: In practical applications up to universe sizes of u = 2^64,
there's no requirement to implement binary search tree due to
locals only managing 64 keys or less, so bitwise leafs suffice.*

## Lazy Allocation
Even with great achievements in saving memory to fit a van Emde Boas 
tree into O(u) bits, it's still not feasible to fully allocate the tree 
upfront (exabyte range, see introduction). This is where lazy 
allocation comes into play.


