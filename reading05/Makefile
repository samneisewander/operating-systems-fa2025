CC=	gcc
CFLAGS=	-Wall -g -std=gnu99
LIBS= -lcrypto -pthread 

test:	
	@$(MAKE) -sk test-quiz test-program

program:    program.o queue.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

queue.o: queue.c
	$(CC) $(CFLAGS) -c -o $@ $^

program.o: program.c
	$(CC) $(CFLAGS) -c -o $@ $^

test-quiz:
	../.scripts/check.py

test-program: program
	curl -sLO https://pnutz.h4x0r.space/courses/cse.30341.fa25/static/txt/reading05/test_program.sh
	chmod +x test_program.sh
	./test_program.sh

clean:
	rm -f program *.o test_program.sh
