CFLAGS := -std=c11 -g \
	-Wall -Wextra -pedantic \
	-Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes \
	-D_GNU_SOURCE \
	$(CFLAGS)

PREFIX = /usr

all: environ
environ: environ.o specifier.o env.o xdg.o util.o

clean:
	$(RM) environ *.o

.PHONY: clean install uninstall
