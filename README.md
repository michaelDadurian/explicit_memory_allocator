### A dynamic memory allocator written in C to replace the standard malloc, realloc and free functions.

In this implementation, it is assumed that each memory row will be 64-bits in size (1 quad word in x86-64). The address of each block must be divisible by 16 to properly align each block to be capable of storing the largest data type in an x86-64 architecture, a long double (16 bytes).


## Memory block headers & footers:

The header of each block is defined as a single 64-bit quad word, and we use structs for casting and representing headers for free and allocated blocks and footers.

The header of each block contains 4 bits of padding size (represented in bytes), 28 unused bits, 28 bits of block size (represented in bytes), and an allocated bit. There are 16 bytes to store the payload. The footer of each block contains the block size in bytes, and an allocated bit representing either free or allocated blocks.


## Properties:

Block Placement Policy: first fit placement.

Management Policy: explicit free list with last-in-first-out policy, as in the most recently freed block becomes the head of the free list.

Coalescing: immediate coalescing using boundary tags.

Splitting Blocks: splits blocks which are too large to reduce internal fragmentation.

## Features:

Keeps track of total internal and external fragmentation, as well as the number of allocations, frees, and coalesces.
