// clang -Wall -Wextra -Og -g3 -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer -fsanitize=unsigned-integer-overflow -DDEBUG -DARENALLOC_STATS example.c -o example

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arenalloc.h"

arena_t a;

int main() {
	//a = arena_new((arena_malfre_t){malloc, free}); // 1 mem page / blk
	//a = arena_new_s((arena_malfre_t){malloc, free}, 128); // 128b of data /blk
	a = arena_new_v((arena_malfre_t){malloc, free}); // 1 blk / alloc

	char str[24], *prev;
	int n;
	fputs("Commands: quit, del, !.\n> ", stdout);
	while(scanf("%23s", str)) {
		n = strlen(str) + 1;
		if (!strcmp(str, "quit"))
			break;
		if (!strcmp(str, "del")) {
			puts("freeing last 100 bytes");
			arena_free_bytes(100, &a);
			arena_print(&a, "one");
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
