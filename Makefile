CC := gcc
CFLAGS := \
	-std=c99 \
	-Wall \
	-Wextra \
	-Werror \
	-Wpedantic \
	-Wcast-align \
	-Wcast-qual \
	-Wmissing-declarations \
	-Wredundant-decls \
	-Wmissing-prototypes \
	-Wpointer-arith \
	-Wshadow \
	-Wstrict-prototypes \
	-Wmissing-field-initializers \
	-fno-common

.PHONY: all
all: twig_test

twig_test: main.c twig.h
	$(CC) $(CFLAGS) -o twig_test main.c

.PHONY: test
test: twig_test
	./twig_test
