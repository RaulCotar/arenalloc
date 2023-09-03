#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arenalloc.h"

arena_t a;

int main() {
	a = arena_new_least((arena_malfre_t){malloc, free}, 20);

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
