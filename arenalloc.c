// SEE LICENSE AT THE BOTTOM
#include <unistd.h>

#ifdef DEBUG
#include <string.h>
#endif

#include "arenalloc.h"

#define MIN(A, B) (((A) <= (B)) ? (A) : (B))
#define MAX(A, B) (((A) >= (B)) ? (A) : (B))

arena_t *_arena_link_live_blk(arena_t *a, arena_blk_t *b) {
	if (!a->beg) a->beg = b;
	if (a->end) a->end->next = b;
	b->prev = a->end;
	b->next = NULL;
	a->end = b;
	#ifdef ARENALLOC_STATS
	a->cur_live_size += b->size;
	a->peak_com_size = MAX(a->peak_com_size, a->cur_live_size+a->cur_free_size);
	a->blk_live++;
	a->blk_peak = MAX(a->blk_peak, a->blk_live + a->blk_free);
	#endif
	return a;
}

arena_blk_t *_arena_unlink_live_blk(arena_t *a, arena_blk_t *b) {
	if (a->beg == b) a->beg = b->next;
	if (a->end == b) a->end = b->prev;
	if (b->prev) b->prev->next = b->next;
	if (b->next) b->next->prev = b->prev;
	#ifdef ARENALLOC_STATS
	a->cur_live_size -= b->size;
	a->blk_live--;
	#endif
	return b;
}

arena_t *_arena_link_free_blk(arena_t *a, arena_blk_t *b) {
	if (!a->fbeg) a->fbeg = b;
	if (a->fend) a->fend->next = b;
	b->prev = a->fend;
	b->next = NULL;
	a->fend = b;
	#ifdef ARENALLOC_STATS
	a->cur_free_size += b->size;
	a->peak_com_size = MAX(a->peak_com_size, a->cur_live_size+a->cur_free_size);
	a->blk_free++;
	a->blk_peak = MAX(a->blk_peak, a->blk_live + a->blk_free);
	#endif
	#ifdef DEBUG
	memset(b->data, 0xc, b->size); // don't behave nicely on RaF
	#endif
	return a;
}

arena_blk_t *_arena_unlink_free_blk(arena_t *a, arena_blk_t *b) {
	if (a->fbeg == b) a->fbeg = b->next;
	if (a->fend == b) a->fend = b->prev;
	if (b->prev) b->prev->next = b->next;
	if (b->next) b->next->prev = b->prev;
	#ifdef ARENALLOC_STATS
	a->cur_free_size -= b->size;
	a->blk_free--;
	#endif
	return b;
}

arena_blk_t *_arena_get_new_blk(size_t size, arena_t *a) {
	arena_blk_t *b = a->malloc(sizeof(arena_blk_t) + size);
	b->size = size;
	return b;
}

arena_blk_t *_arena_get_free_blk(size_t least, arena_t *a) {
	arena_blk_t *i = a->fend, *b = NULL;
	size_t s = -1;
	while (i) {
		if (i->size >= least && i->size - least < s)
			s = i->size - least, b = i;
		i = i->prev;
	}
	if (b) return _arena_unlink_free_blk(a, b);
	else return NULL;
}

arena_t arena_new(arena_malfre_t mf) {
	return arena_new_s(mf, sysconf(_SC_PAGESIZE) - sizeof(arena_blk_t));
}

arena_t arena_new_v(arena_malfre_t mf) {
	return arena_new_s(mf, 0);
}

arena_t arena_new_least(arena_malfre_t mf, size_t least) {
	aa_assert(least);
	long bs = sysconf(_SC_PAGESIZE);
	least = bs / (least + sizeof(arena_blk_t));
	while (least >>= 1)
		bs /= 2;
	return arena_new_s(mf, bs - sizeof(arena_blk_t));
}

arena_t arena_new_s(arena_malfre_t mf, size_t blksize) {
	arena_t a = {
		.beg=0, .end=0, .fbeg=0, .fend=0, .head=0, .blk_size=blksize,
		.malloc=mf.malloc, .free=mf.free
	};
	if (blksize) {
		_arena_link_live_blk(&a, _arena_get_new_blk(blksize, &a));
		a.head = a.end->data;
	}
	return a;
}

arena_t *arena_free(arena_t *a) {
	while(a->end)
		_arena_link_free_blk(a, _arena_unlink_live_blk(a, a->end));
	a->head = NULL;
	#ifdef ARENALLOC_STATS
	a->cur_bytes = 0;
	#endif
	return a;
}

arena_t *arena_decom(arena_t *a) {
	while (a->end) {
		arena_blk_t *t = _arena_unlink_live_blk(a, a->end);
		a->free(t);
		#ifdef ARENALLOC_STATS
		a->blk_decom++;
		#endif
	}
	while (a->fend) {
		arena_blk_t *t = _arena_unlink_free_blk(a, a->fend);
		a->free(t);
		#ifdef ARENALLOC_STATS
		a->blk_decom++;
		#endif
	}
	a->head = NULL;
	return a;
}

arena_t *arena_free_last_blk(arena_t *a) {
	if (!a->end) return a;
	#ifdef ARENALLOC_STATS
	a->cur_bytes -= (size_t)(a->head - ((void*)(a->end->data)));
	#endif
	_arena_link_free_blk(a, _arena_unlink_live_blk(a, a->end));
	if (a->end) a->head = a->end->data + a->end->size - 1;
	else a->head = NULL;
	return a;
}

size_t arena_free_bytes(size_t size, arena_t *a) {
	if (!a->end) return 0;
	size_t left = (size_t)(a->head - ((void*)(a->end->data)));
	if (size >= left) {
		_arena_link_free_blk(a, _arena_unlink_live_blk(a, a->end));
		if (a->end) a->head = a->end->data + a->end->size - 1;
		else a->head = NULL;
		#ifdef ARENALLOC_STATS
		a->cur_bytes -= left;
		#endif
		return left;
	}
	a->head -= size;
	#ifdef ARENALLOC_STATS
	a->cur_bytes -= size;
	#endif
	return size;
}

void *arena_calloc(size_t nmemb, size_t size, arena_t *arena) {
	unsigned __int128 r = (unsigned __int128)nmemb * size;
	if (r > -1) r = -1;
	return arena_alloc((size_t)r, arena);
}

void *arena_alloc(size_t size, arena_t *a) {
	#ifdef ARENALLOC_STATS
	a->tot_allocs++;
	#endif
	if (size > a->blk_size && a->blk_size)
		return NULL; // illegal call
	arena_blk_t *b = a->end;
	if (!b || size > (size_t)(((void*)(b->data)) + b->size - a->head)) {
		b = _arena_get_free_blk(a->blk_size ?: size, a);
		if (b)
			_arena_link_live_blk(a, b);
		else {
			b = _arena_get_new_blk(a->blk_size ?: size, a);
			if (b)
				_arena_link_live_blk(a, b);
			else
				return NULL; // No way to get that memory
		}
		a->head = b->data;
	}
	a->head += size;
	#ifdef ARENALLOC_STATS
	a->cur_bytes += size;
	a->tot_bytes += size;
	#endif
	return a->head - size;
}

#if defined(DEBUG) || !defined(NDEBUG)
#include <stdio.h>
void arena_print(arena_t *a, char *name) {
	printf("ARENA '%s'\nbeg: %p\nend: %p\nfbeg: %p\nfend: %p\nblk_size: %lu "
		"(%lu is data)\nhead: %p\nlive chain:\n", name, a->beg, a->end, a->fbeg,
		a->fend, a->blk_size + sizeof(arena_blk_t), a->blk_size, a->head);
	arena_blk_t *b = a->beg;
	unsigned int i=0;
	while (i++, b) {
		printf("(%u)\tprev: %14p | this: %p | next: %14p | memory: %p + %lu\n",
			i, b->prev, b, b->next, b->data, b->size);
		b =  b->next;
	}
	puts("free chain:");
	b = a->fbeg;
	i=0;
	while (i++, b) {
		printf("(%u)\tprev: %14p | this: %p | next: %14p | memory: %p + %lu\n",
			i, b->prev, b, b->next, b->data, b->size);
		b =  b->next;
	}
	#ifdef ARENALLOC_STATS
	printf("cur_live_size: %lu\ncur_free_size: %lu\npeak_com_size: %lu\n"
		"tot_alloc: %lu\ncur_bytes: %lu\ntot_bytes: %lu\nblk_live: %u\n"
		"blk_free: %u\nblk_decom: %u\nblk_peak: %u\ncomputed sanity: %i\n\n",
		a->cur_live_size, a->cur_free_size, a->peak_com_size, a->tot_allocs,
		a->cur_bytes, a->tot_bytes, a->blk_live, a->blk_free, a->blk_decom,
		a->blk_peak, arena_sanity_check(a));
	#else
	puts("Define ARENALLOC_STATS to enable arena statistics.");
	#endif
}
#elif !defined(DEBUG) || defined(NDEBUG)
void arena_print(arena_t *a, char *name) {(void)a; (void)name;}
#endif

// And now, for the one and only, the **sanity check** [bam bam baaam].
int arena_sanity_check(arena_t *a) {
	if ((a->beg && a->end) != (a->beg || a->end)) return -1;
	if ((a->fbeg && a->fend) != (a->fbeg || a->fend)) return -2;
	if ((a->beg || a->end) && !a->head) return -3;
	if (a->end && (a->head < ((void*)(a->end->data)) ||
		a->head > ((void*)(a->end->data)) + a->end->size)) return -4;
	if (!a->malloc || !a->free) return -5;
	#ifdef ARENALLOC_STATS
	if (a->blk_live + a->blk_free > a->blk_peak) return -6;
	if (a->blk_live + a->blk_free + a->blk_decom > a->tot_allocs) return -7;
	#endif
	size_t cs=0;
	unsigned int lv=0, fr=0;
	arena_blk_t *b = a->beg;
	while(b) {
		if (b->next && b->next->prev != b) return -8;
		if (!b->next && a->end != b) return -9;
		cs += b->size;
		lv++;
		b = b->next;
	}
	#ifdef ARENALLOC_STATS
	if (lv != a->blk_live) return -10;
	if (cs != a->cur_live_size) return -11;
	#else
	(void)cs; (void)lv; (void)fr;
	#endif
	b = a->fbeg; cs = 0;
	while(b) {
		if (b->next && b->next->prev != b) return -12;
		if (!b->next && a->fend != b) return -13;
		cs += b->size;
		fr++;
		b = b->next;
	}
	#ifdef ARENALLOC_STATS
	if (fr != a->blk_free) return -14;
	if (cs != a->cur_free_size) return -15;
	if (a->peak_com_size < a->cur_live_size + a->cur_free_size) return -16;
	#endif
	return 0;
}

#ifdef ARENALLOC_DEF_ARENA
static arena_t _def_arena;
#endif

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