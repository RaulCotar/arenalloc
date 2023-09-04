// SEE LICENSE AT THE BOTTOM
#ifndef _ARENALLOC_H_
#define _ARENALLOC_H_

/*   Arena allocator - commissions memory blocks from the OS and uses them to
 * allocate memory for the program. Freed blocks are not returned to the OS
 * until they are also decommited and can thus be reused by the arena.
 *
 *  Every allocation allocates a contiguous memory range, and thus all
 * allocations must be smaller the block size. This implementation also supports
 * variable block sizes that guarrantee 1 block/allocation (so it becomes a
 * linked list allocator). In general you should either use the variable mode,
 * or have a block size significantly larger than that of individual allocations
 * . Arena block sizes can be changed at any time by setting `arena.blk_size`.
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

#ifndef ARENALLOC_ASSERT
#include <assert.h>
#define ARENALLOC_ASSERT assert
#endif
#define aa_assert ARENALLOC_ASSERT

#include <stddef.h>

typedef struct arena_blk_t arena_blk_t;
struct arena_blk_t {
	arena_blk_t *prev; // prev block in chain, 0=this is first
	arena_blk_t *next; // next block in chain, 0=this is last
	size_t size;
	unsigned char data[];
};

typedef struct {
	void*(*malloc)(size_t);
	void(*free)(void*);
} arena_malfre_t;

typedef struct {
	arena_blk_t *beg, *end; // block in use
	arena_blk_t *fbeg, *fend; // free blocks
	void *head; // points to the free space in the last block
	size_t blk_size; // block size, 0=variable (one blk/alloc)
	void*(*malloc)(size_t);
	void(*free)(void*);
	#ifdef ARENALLOC_STATS
	struct {
		size_t cur_live_size;	// current live size of the arena
		size_t cur_free_size;	// current free size of the arena
		size_t peak_com_size;	// peak commited size of the arena
		size_t cur_bytes; 		// current bytes allocated (without wasted ones)
		size_t tot_bytes;		// total bytes allocated
		size_t tot_allocs;		// total number of allocations
		double avg_alloc_sz;	// average allocation size (in bytes)
		unsigned int blk_live;	// current nr of committed live (in use) blocks
		unsigned int blk_free; 	// current nr of committed free blocks
		unsigned int blk_decom;	// total nr of decommitted blocks
		unsigned int blk_peak; 	// peak nr of committed blocks
	};
	#endif
} arena_t;

#define arena_new arena_new_p
// New arena with blocks that perfectly fit a virtual memory page.
arena_t arena_new_p(void);
// New arena with a variable block size: one block per allocation.
arena_t arena_new_v(arena_malfre_t mf);
// New arena of arbitrary block size.
arena_t arena_new_s(arena_malfre_t mf, size_t blksize);
// New arena with a block size of at least `least`. (usually page size divisor)
arena_t arena_new_least(arena_malfre_t mf, size_t least);

// Free and decommit all blocks in the arena.
arena_t *arena_decom(arena_t *arena);
// Free (not decommit) all blocks in the arena.
arena_t *arena_free(arena_t *arena);
// Free the 'end' block.
arena_t *arena_free_last_blk(arena_t *a);
// Free the last `size` bytes allocated in the arena. Emptied blocks are freed.
// Returns the number of bytes deallocated. (!=size iff trying to free too much)
// Does not cross block boundaries, but emptied blocks are freed. To deallocate
// more memory than what is currently stored in the end block, call this
// function multiple times with its own return value until it returns 0.
size_t arena_free_bytes(size_t size, arena_t *arena);

// Allocate memory in an arena; malloc equivalent.
void *arena_alloc(size_t size, arena_t *arena);
// Allocate memory in an arena; calloc equivalent.
void *arena_calloc(size_t nmemb, size_t size, arena_t *arena);

// Checks arena for dangling pointers, contradictory values, etc., but doesn't
// change anything about the arena. A completely sane arena has a sanity value
// of 0; see function implementation for other return values.
int arena_sanity_check(arena_t *arena);
// Print data about an arena to stdout, but only in debug mode.
void arena_print(arena_t *a, char *name);

#ifdef ARENALLOC_DEF_ARENA
extern arena_t _def_arena;
// TODO: declare macros for using _def_arena without mentioning it
#endif

#endif /* _ARENALLOC_H_ */

/*
Copyright (c) 2023 RaulCotar

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/