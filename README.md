# arenalloc
A simple arena allocator in C, `arenalloc` started off in the [Squid-lang](https://github.com/RaulCotar/Squid-lang) codebase, but has become a stanalone single file library. Current version is 1.0.0.

## Goal
`arenalloc` aims to be simple and decently fast and versatile to be used in small and medium size-sized projects. Memory safety is also an importatnt concern (it's an allocator after all), and currently no bugs or memory leaks are known (tested with example.c, UBSan and ASan).

## Documentation
Documentation can be found inside [arenalloc.h](./arenalloc.h). The following is an excerpt from the top of the file:
```c
/*   Arena allocator - commissions memory blocks from the OS and uses them to
 * allocate memory for the program. Freed blocks are not returned to the OS
 * until they are also decommited and can thus be reused by the arena.
 *
 *  Every allocation allocates a contiguous memory range, and thus all
 * allocations must be smaller the block size. This implementation also supports
 * variable block sizes that guarrantee 1 block/allocation (so it becomes a
 * linked list allocator).
 *
 *   In general, you can only free an entire block at a time, but this
 * implementation also supports freeing the last N allocated bytes in an arena.
 *
 *   This implementation does not rely on a specific memory allocator (such as
 * malloc), but instead, each arena has a pointer to an allocator function which
 * is used to commission blocks. That being said, I've only tested it with the
 * malloc implementations provided by Clang and GCC on x64 Linux.
 *
 *  arenalloc is modular and easily extendable if more features are desired down
 * the road.
 *
 * Macro config:
 *	 ARENALLOC_ASSERT - controls the `assert` function used (`assert` by def)
 *	 ARENALLOC_STATS - keep allocation statistics per arena (undef by default)
 *	 ARENALLOC_DEF_ARENA - declare a static arena and macro  function wrappers
 * to use it by default. (undefined by default)
*/
```

## Planned features
- better support for different backends
- snapshot system
- defragmentation system
- proper testing

## Compiling
All you need to do in compile `arenalloc.c` with a compiler that supports `gnu89`.
An example compile command can be found at the top of [example.c](./example.c).

## Licensing
`arenalloc` is destributed under the MIT license. See [LICENSE](./LICENSE).
