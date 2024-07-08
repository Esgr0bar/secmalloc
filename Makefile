CC = gcc
CFLAGS = -I./include -Wall -Wextra -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Werror -z noexecstack -fstack-protector-strong -std=c99
PRJ = my_secmalloc
OBJS = src/my_secmalloc.o	\
	   src/utils/my_free.o	\
	   src/utils/my_calloc.o \
	   src/utils/my_realloc.o \
	   src/utils/initialize.o  \
	   src/utils/log.o

SLIB = lib${PRJ}.a
LIB = lib${PRJ}.so

all: ${LIB}

${LIB} : CFLAGS += -fpic -shared
${LIB} : ${OBJS}

${SLIB}: ${OBJS}

dynamic: CFLAGS += -DDYNAMIC
dynamic: ${LIB}

my_secmalloc_exe: src/my_secmalloc.o $(OBJS)
	$(CC) $(CFLAGS)  -o $@ $^

static: ${SLIB}

clean:
	${RM} src/.*.swp src/*~ src/*.o test/*.o src/utils/*.o

distclean: clean
	${RM} ${SLIB} ${LIB}

build_test: CFLAGS += -DTEST
build_test: ${OBJS} test/test.o
	$(CC) -o test/test $^ -lcriterion -Llib

test: build_test
	LD_LIBRARY_PATH=./lib valgrind test/test


.PHONY: all clean build_test dynamic test static distclean

%.so:
	$(LINK.c) -shared $^ $(LDLIBS) -o $@

%.a:
	${AR} ${ARFLAGS} $@ $^
