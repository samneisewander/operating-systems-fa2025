# Configuration

CC=		gcc
CFLAGS=		-Wall -g -std=gnu99 -Og -Iinclude $(shell curl-config --cflags)

LD=		gcc
LDFLAGS=	-Llib
LIBS=		$(shell curl-config --libs) -pthread -lncurses -lform -lcrypto

AR=		ar
ARFLAGS=	rcs

# Variables

SMQ_HEADERS=	$(wildcard include/smq/*.h)
SMQ_SOURCES=	$(wildcard src/*.c)
SMQ_OBJECTS=	$(SMQ_SOURCES:.c=.o)
SMQ_LIB=	lib/libsmq.a

CHAT_APP=	bin/saichat

TEST_SOURCES= 	$(wildcard tests/test_*.c)
TEST_OBJECTS= 	$(TEST_SOURCES:.c=.o)
TEST_PROGRAMS= 	$(subst tests,bin,$(basename $(TEST_OBJECTS)))
TEST_SCRIPTS=   $(subst bin/,,$(basename $(shell ls bin/test_*.sh)))

UNIT_SOURCES=   $(wildcard tests/unit_*.c)
UNIT_OBJECTS=   $(UNIT_SOURCES:.c=.o)
UNIT_PROGRAMS=  $(subst tests,bin,$(basename $(UNIT_OBJECTS)))
UNIT_SCRIPTS=   $(subst bin/,,$(basename $(shell ls bin/unit_*.sh)))

# Rules

all:	 $(CHAT_APP)

%.o:		%.c $(SMQ_HEADERS)
	@echo "Compiling $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(SMQ_LIB):	$(SMQ_OBJECTS)
	@echo "Linking   $@"
	@$(AR) $(ARFLAGS) $@ $^

$(CHAT_APP): src/saichat.o $(SMQ_LIB) 
	@echo "Linking $@"
	@$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

bin/%:  	tests/%.o $(SMQ_LIB)
	@echo "Linking   $@"
	@$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

test:		$(TEST_PROGRAMS)
	@$(MAKE) -sk test-all

test-all:   	test-units test-scripts

test-units:	$(UNIT_PROGRAMS) $(UNIT_SCRIPTS)

unit_%:		bin/unit_% bin/unit_%.sh
	@./bin/$@.sh

test-scripts:	$(TEST_PROGRAMS) $(TEST_SCRIPTS)

test_%:		bin/test_% bin/test_%.sh
	@./bin/$@.sh

clean:
	@echo "Removing objects"
	@rm -f $(SMQ_OBJECTS) $(UNIT_OBJECTS) $(TEST_OBJECTS)

	@echo "Removing libraries"
	@rm -f $(SMQ_LIB)

	@echo "Removing saichat"
	@rm -f $(CHAT_APP)

	@echo "Removing test programs"
	@rm -f $(UNIT_PROGRAMS) $(TEST_PROGRAMS)

.PRECIOUS: %.o
