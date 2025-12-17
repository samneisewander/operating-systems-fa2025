/* Reading 07 */

#include <stdio.h>
#include <stdlib.h>

/*
 * // Snippet A
for (int i = 1; i < argc; i++) {
    char *copy = strdup(argv[i]);
    puts(copy);
}

// Snippet B
char buffer[BUFSIZ];
puts(buffer);

// Snippet C
char *s = "string";
*s = 'S';

// Snippet D
char *s = "string";
free(s);

// Snippet E
char *buffer = calloc(1, strlen(argv[0]));
strcat(buffer, argv[0]);
puts(buffer);
free(buffer);
*/

/*segfault
buffer overflow
uninitialized read
memory leak
dangling pointer
double free
invalid free*/


int main(int argc, char *argv[]) {

    puts("memory leak");
    puts("uninitialized read");
    puts("segfault");
    puts("invalid free");
    puts("buffer overflow");

    return EXIT_SUCCESS;
}
