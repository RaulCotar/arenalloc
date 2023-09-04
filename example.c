// clang -Wall -Wextra -Og -g3 -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer -fsanitize=unsigned-integer-overflow -DDEBUG -DARENALLOC_STATS example.c arenalloc.c -o example

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arenalloc.h"

arena_t a;

int main() {
	a = arena_new(); // macro, same as arena_new_p
	//a = arena_new_p(); // 1 mem page / blk
	//a = arena_new_s((arena_malfre_t){malloc, free}, 128); // 128b of data /blk
	// a = arena_new_v((arena_malfre_t){malloc, free}); // 1 blk / alloc

	char str[24], *prev;
	int n;
	fputs("Commands: quit, free, blk_size, !.\n> ", stdout);
	while(scanf("%23s", str)) {
		n = strlen(str) + 1;
		if (!strcmp(str, "quit"))
			break;
		else if (!strcmp(str, "blk_size")) {
			size_t s;
			scanf(" %lu", &s);
			if (s) a.blk_size = s, arena_print(&a, "one");
			else puts("Bad input!");
		}
		else if (!strcmp(str, "free")) {
			size_t s;
			scanf(" %lu", &s);
			if (s)
				printf("Freed %lu bytes.\n", arena_free_bytes(s, &a)),
				arena_print(&a, "one");
			else puts("Bad input!");
		}
		else if (!strcmp(str, "!"))
			puts(prev);
		else {
			printf("%s (%i+1)\n", str, n-1);
			fflush(stdout);
			prev = memcpy(arena_alloc(n, &a), str, n);
			arena_print(&a, "one");
		}
		fputs("\n> ", stdout);
	}
	arena_free_last_blk(&a);
	arena_free(&a);
	arena_decom(&a);

	return 0;
}
