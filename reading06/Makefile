CC=	gcc
CFLAGS=	-Wall -g -std=gnu99
LIBS=	-pthread -lcrypto

test:	
	@$(MAKE) -sk test-quiz test-program

program:    program.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

test-quiz:
	../.scripts/check.py

test-program: program
	curl -sLO https://pnutz.h4x0r.space/courses/cse.30341.fa25/static/txt/reading06/test_program.sh
	chmod +x test_program.sh
	./test_program.sh

clean:
	rm -f program
