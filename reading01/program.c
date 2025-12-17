/* Reading 01 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
    
    size_t questions = 12;
    bool answers[] = {
        false,
        false,
        false,
        true,
        false,
        true,
        false,
        true,
        true,
        false,
        true,
        false,
    };

    for (int i = 0; i < questions; i++) {
        printf("%s\n", answers[i] ? "True" : "False");
    }

    return EXIT_SUCCESS;
}
