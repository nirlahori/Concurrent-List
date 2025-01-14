# Concurrent-List

Concurrent List is a container which implements the double linked list. The container allows threads to perform operations on the
same instance simultaneously. The List allows the operations to be executed at the front, back as well as in between of the nodes.
The functionality is implemented using synchronization primitives such as mutexes instead of atomic transactions.

# Design

The Concurrent List achieves the concurrent functionality by locking adjacent nodes of the node on which the operation is to be 
performed. For an instance, if the 19th node from the list is to be removed then the container will lock the node 18 and node 20.
This ensures that no other thread is performing an operation which involves node 18 or 20. The node 19 can be safely deattached from
the list and the remaining adjacent nodes can be safely repointed to each other.

The List is currently under active development and as such I am continously trying to improve the implementation while also trying
to increase the capabilities of the container.
