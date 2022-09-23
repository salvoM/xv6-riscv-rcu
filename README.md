# Xv6-riscv-rcu
Supervisor: Prof. Vittorio Zaccaria

This is a project developed for the PoliMI *Advanced Operating Systems* course (2021/22).

## Project
The goal of the project is the implementation of **RCU locking for some kernel data in xv6**.
During the initial phase of the project we decided, together with the advisor, that we would be implementing RCU locking on the processes list managed by the xv6’s Kernel.

## What is RCU?
> Read-copy update (RCU) is a synchronization mechanism that achieves scalability improvements by allowing reads to occur concurrently with updates. In contrast with conventional locking primitives that ensure mutual exclusion among concurrent threads regardless of whether they be readers or updaters, or with reader-writer locks that allow concurrent reads but not in the presence of updates, RCU supports concurrency between a single updater and multiple readers. RCU ensures that reads are coherent by maintaining multiple versions of objects and ensuring that they are not freed up until all pre-existing read-side critical sections complete. RCU defines and uses efficient and scalable mechanisms for publishing and reading new versions of an object, and also for deferring the collection of old versions. These mechanisms distribute the work among read and update paths in such a way as to make read paths extremely fast. 
>
> -- [LWN](https://lwn.net/Articles/262464/)

## What is Xv6?
Xv6 is a teaching operating system developed in the summer of 2006 for MIT's Operating System Engineering course.
It is a modern reimplementation of Sixth Edition Unix.

# Authors
- Nicolas Gallo
nicolas.gallo@mail.polimi.it
- Salvo Maccarrone
salvatore.maccarrone@mail.polimi.it