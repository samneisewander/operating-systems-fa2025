CC=	gcc
CFLAGS=	-Wall -g -std=gnu99 -pthread $(shell curl-config --cflags)
LIBS=	$(shell curl-config --libs)

test:
	@$(MAKE) -sk test-program

program:    program.c queue.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

test-program: program
	curl -sLO https://pnutz.h4x0r.space/courses/cse.30341.fa25/static/txt/exam02/test_program.sh
	chmod +x test_program.sh
	./test_program.sh

clean:
	rm -f program
