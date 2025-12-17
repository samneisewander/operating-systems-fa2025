#!/bin/sh

for i in $(seq 1 11); do
    n=$(printf "%02d" $i)
    mkdir reading$n

    cat > reading$n/README.md <<EOF
# Reading $n
EOF

    cat > reading$n/Makefile <<EOF
CC=	gcc
CFLAGS=	-Wall -g -std=gnu99
LIBS=	

test:	
	@\$(MAKE) -sk test-quiz test-program

program:    program.c
	\$(CC) \$(CFLAGS) -o \$@ $^ \$(LIBS)

test-quiz:
	../.scripts/check.py

test-program: program
	curl -sLO https://pnutz.h4x0r.space/courses/cse.30341.fa25/static/txt/reading$n/test_program.sh
	chmod +x test_program.sh
	./test_program.sh

clean:
	rm -f program
EOF
    cat > reading$n/program.c <<EOF
/* Reading $n */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    return EXIT_SUCCESS;
}
EOF
done

for p in $(seq 1 4); do
    n=$(printf "%02d" $p)
    mkdir project$n
    cat > project$n/README.md <<EOF
# Project $n
EOF
done
