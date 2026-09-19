# Case 13 - Intrusive Linked Structures: Ownership, Node Embedding and Specialized List Designs

## Table of Contents

- [Introduction](#introduction)
- [Overview](#overview)
- [Design Space](#design-space)
- [BlList](#bl-list)
- [BlDirectList](#bl-directlist)
- [BlForwardOwnerList](#blforwardownerlist)
- [BlDirectListOwner](#bldirectlistowner)
- [Comparison With Standard Library Containers](#comparison-with-standard-library-containers)
- [`BlList` vs `std::list`](#bl-list-vs-std-list)
- [`BlDirectList` vs `std::list<T*>`](#bl-direct-list-vs-std-list-t)
- [`BlForwardOwnerList` vs `std::forward_list`](#bl-forward-owner-list-vs-std-forward-list)
- [`BlDirectListOwner` vs `std::forward_list<T*>`](#bl-direct-list-owner-vs-std-forward-list-t)
- [Why Use These Structures Instead of `std::`?](#why-use-these-or-std)
- [Why Not Always Use a Linked List?](#why-not-use-llist)
- [Choosing Between the Four](#choosing-between-the-four)
- [Advantages and Costs](#advantages-and-costs)
- [Structural Verification](#structural-verification)
- [An Important Result From Testing](#an-important-result-from-testing)
- [General Production Lesson](#general-production-lesson)
- [Owner-Location Pointers as a General Technique](#owner-lp-general-technique)
- [Final Perspective](#final-perspective)

## Introduction

Four implementations specifically addressing the linked list problem were developed, resulting in four structural classes designed to replace standard `std::` operations (**In very specific production cases**).

```cpp
    std::list<T>
    std::forward_list<T>
    std::vector<T>
    std::deque<T>
```

While the standard solutions are excellent, they suffer from issues - such as `overhead`, `heap memory fragmentation`, and `constructor complexity` - that can compromise the external structures and classes used in advanced systems (particularly in production environments like `gameplay`, `server/client interfaces`, and `simulations`) requiring absolute resource control.

In these contexts, the need for efficiency and code specialization outweighs the benefits of generality; although these custom implementations solve the same problems as `std::`, the specific list designs presented here can significantly improve overall code maintainability as well as the efficiency of accessing, copying, and removing elements from dynamic lists.

Previous cases examined comparisons and replacements for data types from `std::` or other standard libraries that offer general solutions to general problems, this case is no exception. The goal here is to reduce the excessive overhead and heap fragmentation associated with certain fundamental `std::` classes, providing optimized, highly simplified solutions ready for direct use in production—without the risk of data corruption or excessive memory consumption.

## Overview

This case explores four specialized linked-list implementations designed around different combinations of node ownership, node representation, and traversal requirements:

* `BlList<T>`
* `BlDirectList<T, ...>`
* `BlForwardOwnerList<T>`
* `BlDirectListOwner<T, ...>`

> **A data structure can become substantially more efficient for a specific workload by eliminating capabilities, indirections, allocations, and state that the workload does not require.**

The four implementations represent different points in the design space between:

* Internally managed nodes vs externally managed nodes.
* Doubly linked vs singly linked structures.
* Direct object storage vs intrusive node embedding.
* Conventional predecessor links vs owner-location back-links.
* General-purpose containers vs specialized low-level data structures.

All four implementations were subjected to structural tests covering insertion, removal, ordering, ownership, reuse, cross-list misuse, random operations, and structural invariants.

The objective of these tests was to demonstrate that, despite performing millions of operations involving lists and real-world environment simulations, the four systems remained stable throughout every single test - with no memory leaks, proper fragmentation management, and absolutely no data loss across more than 10 million operations.

---

## Design Space

The four structures can be understood through two independent questions.

### Who owns the node?

```text
Internal node ownership
    |
    +-- BlList
    |
    +-- BlForwardOwnerList

External node ownership
    |
    +-- BlDirectList
    |
    +-- BlDirectListOwner
```

### Does the structure require backward traversal?

```text
Doubly linked
    |
    +-- BlList
    +-- BlDirectList

Singly linked
    |
    +-- BlForwardOwnerList
    +-- BlDirectListOwner
```

This produces the following matrix:

| Container              | Linking                        | Node ownership | Node representation               |
| ---------------------- | ------------------------------ | -------------- | --------------------------------- |
| `BlList<T>`            | Doubly linked                  | Internal       | Separate list node containing `T` |
| `BlDirectList<T>`      | Doubly linked                  | External       | User object is the node           |
| `BlForwardOwnerList<T>`       | Singly linked + owner backlink | Internal       | Separate list node containing `T` |
| `BlDirectListOwner<T>` | Singly linked + owner backlink | External       | User object is the node           |

The important observation is that these are not four unrelated implementations.

They progressively remove functionality that may not be required.

---

## `BlList`   <a id="bl-list"></a>

`BlList<T>` is the most conventional design of the four.

```cpp
template <typename T>
class BlList//alignment 8 bytes
{
    BlListNode<T>* pHead;
    BlListNode<T>* pTail;
    BlListNode<T>* pDequeued;
    int64 nCountOfElements;
}
```

Each element is stored inside an internally managed node containing:

```cpp
template <typename T>
class BlListNode
{
    void* pCurrentList;
    BlListNode<T>* pNext;
    BlListNode<T>* pBack;
    T hData;
}
```

Conceptually:

```text
List
 |
 +--> Node1 (head)
 |      +-- list identity
 |      +-- next
 |      +-- back -> nullptr
 |      +-- T data
 |
 +--> Node2
 ...
 +--> NodeN (tail)
        ...
        +-- next -> nullptr
        ...
```

The list controls the lifetime of its nodes and can use its own allocation/pooling strategy.

### Advantages

### Internal ownership

The user does not need to manually allocate or release individual list nodes.

This simplifies the API and reduces the possibility of ownership mistakes at the call site.

### Doubly linked traversal

Each node contains both forward and backward links.

This makes the structure appropriate when operations require navigation in both directions.

### Custom allocation strategy

Because node allocation is controlled by the container, the implementation can use pooling or other specialized memory-management strategies rather than relying on a general-purpose allocation for every node.

This can be valuable in workloads involving frequent insertion and removal of small objects.

### Explicit list membership

Each node carries a list identity marker.

This allows the implementation to reject operations involving nodes that already belong to another list.

### Disadvantages

The node contains more structural information:

```text
current-list
next
back
value
```

This increases per-element overhead.

The structure also retains the normal cache-locality disadvantages of linked structures: nodes are not necessarily contiguous in memory.

The current implementation is deliberately designed around trivial copy/destruction semantics because node data is manipulated as raw storage rather than through a general-purpose object lifetime model.

---

## `BlDirectList`   <a id="bl-directlist"></a>

`BlDirectList` changes the ownership model.

```cpp
template <typename _Ty, _Ty* _Ty::* NextField, _Ty* _Ty::* BackField, void* _Ty::* CurrentListField>
class BlDirectList//alignment 8 bytes
{
    _Ty* pHead;
    _Ty* pTail;
    int64 nCount;
}
```

**The list doesn't create the nodes.**

Instead, the object itself contains the intrusive linkage fields.

Conceptually:

```text
Object
 |
 +-- next
 +-- back
 +-- list identity
 +-- application data
```

**The object is the node.**

There is no separate wrapper node containing a pointer to the object.

> Unlike `BlLists`, these types of direct lists are used to manage external objects as if they were nodes, they require only three fields within the external classes being used — `NextField`, `BackField`, and an auxiliary field (such as `void*`,  `CurrentListField`) to indicate membership. Thanks to this last field, it is possible to determine in `O(1)` time whether a BlDirectList node object belongs to a specific list or another one.

```cpp
inline bool CheckNodeInList(_Ty* _Node) const
{
	return (_Node->*CurrentListField == this);
}
```

> **NOTE: If the _Node->\*CurrentListField field is `nullptr`, it means it does not belong to a list, if it is not `nullptr`, two scenarios are possible: it is `== this` (belonging to the current list under test) or `!= this` (belonging to a different list). Care must be taken to manage such fields properly from the outside to avoid inconsistencies within the `BlDirectList` structure during operations.**

### Advantages

### No additional list node

The list operates directly on the existing object.

This removes an additional level of indirection and avoids allocating a separate node solely for list membership.

### External lifetime management

The lifetime of the object can be controlled by another subsystem.

For example, an object can belong to:

* A memory pool.
* An engine subsystem.
* A resource manager.
* A scheduler.
* An object registry.

The list does not need to own or destroy it.

### Strong integration with intrusive systems

An object can contain multiple sets of linkage fields and consequently participate in multiple independent intrusive structures.

For example:

```text
Object
 |
 +-- list A links
 |
 +-- list B links
 |
 +-- application state
```

This is useful in systems where an object has several independent organizational relationships.

> In production lists, you might want to sort items by name, type, size, location, etc. Does that ring a bell? With this type of list, you can achieve that using a single structure.

### Disadvantages

The lifetime responsibility is transferred to the caller.

Removing an object from the list does not imply destroying the object.

The object also becomes structurally coupled to the list implementation because it must contain the required intrusive fields.

This is a deliberate trade-off:

> **less container overhead in exchange for stronger coupling and greater lifetime responsibility.**

> Here, the programmer must be very careful about how the memory for each element is handled. The list provides control and order, but the programmer (and, by extension, the code external to the list) is responsible for managing the variable's lifecycle.

---

## `BlForwardOwnerList`

`BlForwardOwnerList` removes the backward link.

```cpp
template <typename T>
struct BlFONode
{
    void* pCurrentList;
	BlFONode<T>* pNext;
	BlFONode<T>** pOwner;
	T hData;
}

template <typename T>
class BlForwardOwnerList//alignment 8 bytes
{
	BlFONode<T>* pHead;
	BlFONode<T>** pTailOwner;
	BlFONode<T>* pDequeued;
	int64 nCountOfElements;
}
```

Instead of:

```text
A <-> B <-> C
```

the structure is:

```text
A -> B -> C
```

However, every node contains an owner backlink.

Conceptually:

```text
A.owner -> &head
B.owner -> &A.next
C.owner -> &B.next
```

The owner pointer does not identify merely the previous node.

It identifies the **memory location that currently owns the pointer to the node**.

This is an important distinction.

> These lists, which manage internal memory, are particularly interesting when the goal is to eliminate any references to the node within the `Next` and `Back` elements (where applicable), this makes them highly efficient in scenarios where every operation must be carefully managed. While this level of care should theoretically be applied in all cases, the presence of a `BlFONode<T>** pOwner` field ensures that `Next` and `Back` pointers are updated precisely when a node is deleted or inserted, thereby minimizing the risk of infinite lists.

---

## Owner-Location Backlinks

Consider:

```text
head -> A -> B -> C
```

The ownership relationships are:

```text
A.owner -> &head
B.owner -> &A.next
C.owner -> &B.next
```

Therefore, removing `B` does not require finding `A` first.

The operation can conceptually be expressed as:

```cpp
*B.owner = B.next;
```

The node directly knows which pointer must be modified to remove it from the chain.

This is the main structural idea behind the owner-list implementations.

> `Insertion` and `deletion` (disregarding memory allocation, considering only list movement) are of order `O(1)`.

---

## Why Remove the Backward Link?

A conventional doubly linked list stores:

```text
next
back
```

If backward traversal is never required, the `back` field is unnecessary.

Removing it reduces the structural state maintained per node.

The owner backlink provides a different capability:

```text
previous node
```

is replaced conceptually by:

```text
address of the pointer that owns this node
```

This can preserve efficient direct removal without requiring a conventional `previous` pointer.

The trade-off is that backward traversal is no longer available.

> It certainly adds complexity, but there are gains in performance and security.

---

## `BlDirectListOwner`

`BlDirectListOwner` combines the two strongest specializations:

```cpp
template <typename _Ty, _Ty* _Ty::* NextField, _Ty** _Ty::* OwnerField, void* _Ty::* OwnerListField>
class BlDirectListOwner//alignment 8 bytes
{
    _Ty* pHead;
    _Ty* pTail;
    int64 nCount;
}

```

```text
intrusive node
+
singly linked structure
+
owner-location backlink
```

The object itself is the node.

Conceptually:

```text
Object A
 |
 +-- next
 +-- owner
 +-- list identity

Object B
 |
 +-- next
 +-- owner
 +-- list identity
```

The resulting chain is:

```text
head
 |
 v
A -> B -> C -> D -> nullptr
```

with:

```text
A.owner -> &head
B.owner -> &A.next
C.owner -> &B.next
D.owner -> &C.next
```

The list itself only needs its principal structural state:

```text
pHead
pTail
nCount
```

### Advantages

### No wrapper node

The existing object is directly linked.

### No backward pointer

Only forward traversal is represented.

### Direct node removal

The owner-location backlink allows the node to update the pointer that currently owns it.

### External lifetime

The object can be managed by an external allocator, pool, subsystem, or lifetime manager.

### Minimal structural state

The implementation deliberately maintains only the state required by its operations.

This makes `BlDirectListOwner` the most specialized of the four structures.

> Similar to `BlDirectList`, it requires external control by the programmer. However, it offers all the advantages of `BlForwardOwnerList`, including its security and efficiency.

---

## Comparison With Standard Library Containers

These structures should not be viewed as universal replacements for the Standard Library containers.

The Standard Library provides general-purpose containers with well-defined interfaces and object-lifetime semantics.

The custom structures instead optimize for a narrower contract.

---

## `BlList` vs `std::list`   <a id="bl-list-vs-std-list"></a>

Both are doubly linked structures.

`std::list` provides a general-purpose standard interface, allocator support, iterators, object construction/destruction semantics, and integration with the rest of the Standard Library.

`BlList` instead provides direct control over:

* Node representation.
* Allocation strategy.
* Membership tracking.
* Specialized operations.
* The lifetime assumptions of the stored type.

The custom implementation becomes interesting when those specific properties are requirements of the surrounding system.

---

## `BlDirectList` vs `std::list<T*>`   <a id="bl-direct-list-vs-std-list-t"></a>

This is a particularly important comparison.

With:

```cpp
std::list<T*>
```

the conceptual representation is:

```text
list node
    |
    v
  T object
```

With `BlDirectList`:

```text
T object
 |
 +-- next
 +-- back
 +-- list identity
```

The object itself is the node.

This can eliminate an additional node allocation and an additional level of indirection.

The price is that `T` must participate directly in the intrusive representation.

---

## `BlForwardOwnerList` vs `std::forward_list`   <a id="bl-forward-owner-list-vs-std-forward-list"></a>

Both are singly linked.

The important distinction is the owner-location backlink.

A conventional singly linked structure primarily knows:

```text
next
```

while `BlForwardOwnerList` additionally knows:

```text
address of the pointer that owns this node
```

This enables direct node removal without first locating the predecessor.

The additional owner pointer is therefore exchanged for the removal capability.

---

## `BlDirectListOwner` vs `std::forward_list<T*>`   <a id="bl-direct-list-owner-vs-std-forward-list-t"></a>

The representation becomes:

```text
std::forward_list<T*>

list node
    |
    v
  T object
```

versus:

```text
BlDirectListOwner

T object
 |
 +-- next
 +-- owner
 +-- list identity
```

The latter avoids a separate list node and allows the object's own storage to participate directly in the structure.

This is particularly relevant when object lifetime and memory allocation are already managed elsewhere.

---

## Why Use These Structures Instead of `std::`?   <a id="why-use-these-or-std"></a>

The important argument is not:

> "custom containers are always faster."

That would be incorrect.

The correct argument is:

> **A specialized structure can avoid costs that a general-purpose container must retain because those capabilities are part of its general contract.**

For example, if a system already has:

```text
500,000 objects
+
custom memory pool
+
externally controlled lifetime
+
frequent insertion/removal
+
no need for backward traversal
```

then an intrusive singly linked structure can avoid allocating separate list nodes.

Likewise, if the system needs backward traversal but the objects already exist externally, an intrusive doubly linked structure can avoid the wrapper node.

The advantage comes from matching the representation to the workload.

---

## Why Not Always Use a Linked List?   <a id="why-not-use-llist"></a>

Because linked lists are not automatically faster.

For sequential processing, a contiguous structure such as:

```cpp
std::vector<T>
```

can have significant locality advantages.

A vector provides:

* Contiguous memory.
* Efficient cache utilization.
* Straightforward iteration.
* Predictable memory access.

A linked structure instead introduces pointer chasing.

Therefore the correct design question is not:

> "Which container is faster?"

It is:

> **"What operations dominate this workload, and what representation minimizes the cost of those operations?"**

If the workload is dominated by sequential iteration, contiguous storage may be preferable.

If the workload is dominated by insertion/removal of existing objects without moving neighboring objects, linked structures become more attractive.

---

## Choosing Between the Four

A practical decision matrix is:

| Requirement                                       | Structure           |
| ------------------------------------------------- | ------------------- |
| Internal node ownership + bidirectional traversal | `BlList`            |
| External node ownership + bidirectional traversal | `BlDirectList`      |
| Internal node ownership + forward traversal       | `BlForwardOwnerList`       |
| External node ownership + forward traversal       | `BlDirectListOwner` |

Another way to visualize the design is:

```text
                         Node ownership
                    Internal          External

Doubly linked       BlList            BlDirectList

Singly linked       BlForwardOwnerList       BlDirectListOwner
```

This is the central relationship between the four implementations.

---

## Advantages and Costs

| Structure           | Main advantage                                   | Main cost                                             |
| ------------------- | ------------------------------------------------ | ----------------------------------------------------- |
| `BlList`            | Simple ownership model + bidirectional traversal | Larger node and linked-memory behavior                |
| `BlDirectList`      | No wrapper node + bidirectional traversal        | Intrusive representation + external lifetime          |
| `BlForwardOwnerList`       | No backward link + direct node removal           | Additional owner pointer + no backward traversal      |
| `BlDirectListOwner` | Minimal intrusive singly-linked representation   | Highest coupling and external lifetime responsibility |

None of these designs is universally superior.

Each removes a different kind of overhead.

---

## Structural Verification

The implementations were tested independently using deterministic stress tests and an external reference model.

The validation included:

* Empty-list behavior.
* Large front insertions.
* Large back insertions.
* Alternating front/back insertion.
* Insertion before existing nodes.
* Insertion after existing nodes.
* Random node removal.
* Front removal.
* Back removal.
* Clear.
* Reuse after clearing.
* Cross-list ownership violations.
* Double insertion attempts.
* Ownership/backlink validation.
* Cycle detection.
* Duplicate-node detection.
* Exact element ordering.
* Exact node count.
* Head/tail consistency.
* Deterministic random stress.
* `Swap` operations where supported.

The direct/intrusive implementations were tested using externally allocated nodes, reflecting their actual lifetime contract.

Nodes removed from the structures were explicitly released by the test harness rather than by the container.

This distinction is important: **the intrusive containers manipulate membership but do not own object lifetime.**

---

## An Important Result From Testing

The structural tests were not limited to verifying that values appeared in the expected order.

They also validated the internal invariants connecting:

```text
next
owner
list identity
head
tail
count
```

This exposed an issue in `BlDirectListOwner::Swap()` involving the maintenance of `pHead` when exchanging nodes at the beginning of the list.

The issue was not necessarily visible from a simple value-oriented test because the exchanged nodes could still contain apparently valid data.

The structural validator instead detected that the resulting chain did not preserve the expected ownership/link relationship.

This demonstrates an important property of intrusive structures:

> **Testing only the externally visible sequence is insufficient. The structural invariants of the intrusive representation must also be tested.**

---

## General Production Lesson

The most important lesson from these four implementations is broader than linked lists.

> **Do not optimize a data structure by making it more complicated. Optimize it by removing everything the workload does not require.**

If backward traversal is unnecessary, remove the backward link.

If the object already exists, do not allocate another node merely to organize it.

If direct node removal is required in a singly linked structure, store the location that owns the node instead of storing an unnecessary full predecessor relationship.

If memory is controlled by a dedicated subsystem, do not necessarily force the container to become the owner of that memory.

This leads to a general design principle:

```text
Required operations
        ↓
Required state
        ↓
Required representation
        ↓
Required memory management
```

Rather than starting with a general-purpose data structure and accepting all of its costs, the structure can be derived from the actual requirements.

---

## Owner-Location Pointers as a General Technique   <a id="owner-lp-general-technique"></a>

One of the more general ideas explored by these structures is the owner-location pointer.

Instead of storing:

```text
pointer to previous node
```

a node can store:

```text
pointer to the memory location containing the pointer that owns it
```

For:

```text
head -> A -> B -> C
```

the relationships become:

```text
A.owner -> &head
B.owner -> &A.next
C.owner -> &B.next
```

This technique is not limited to linked lists.

The same conceptual mechanism can be useful in specialized:

* Intrusive queues.
* Free lists.
* Tree structures.
* Scheduler queues.
* Resource registries.
* Hash bucket chains.
* Memory-management structures.
* Engine-level object organization.

It is a general technique for turning a relationship into a directly addressable mutation point.

---

## Final Perspective

These four containers are not intended to compete with the Standard Library by providing a universally superior list implementation.

Their purpose is different.

They demonstrate how the representation of a data structure can be derived from the exact requirements of a system.

```text
BlList
    general internal doubly linked structure

BlDirectList
    intrusive doubly linked structure

BlForwardOwnerList
    internal singly linked structure
    with owner-location backlinks

BlDirectListOwner
    intrusive singly linked structure
    with owner-location backlinks
```

The progression is deliberate:

```text
             Generality
                 |
                 v
              BlList
                 |
          remove unused
          backward state
                 |
                 v
        BlForwardOwnerList
                 |
         externalize node
                 |
                 v
         BlDirectListOwner
```

The resulting structures become increasingly specialized.

That specialization is both their main strength and their main cost.

The more the implementation assumes about the workload, memory model, object lifetime, and element type, the more opportunity it has to remove unnecessary machinery.

But the more responsibility is consequently transferred from the container to the programmer.

The fundamental engineering trade-off is therefore:

> **General-purpose abstractions minimize user responsibility. Specialized structures minimize representation and operational overhead for a known workload.**

The appropriate choice depends on which side of that trade-off the system actually needs.

> **NOTE**: type `T` must be trivially copyable.
> It must pass the following checks. 
>
> `static_assert(std::is_trivially_copyable_v<T>);`
> `static_assert(std::is_trivially_destructible_v<T>);`
>
> If it fails to do so, an advanced destructor and constructor for the internal type T must be implemented within `BlList<T>`, to keep the list as simple as possible, it is recommended to use `is_trivially_copyable_v<T>`.
>
> And where possible - though it is not mandatory - maintain a standard layout without inheritance.
>
> `static_assert(std::is_standard_layout_v<T>);`
>
> This maintains a completely controlled structure both inside and outside the code.

## **The example code within the Source folder contains the four complete classes for these lists, ready for immediate use in production.**