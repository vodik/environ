CFLAGS := -std=c99 \
	-Wall -Wextra -pedantic \
	-Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes \
	-D_GNU_SOURCE \
	${CFLAGS}

PREFIX = /usr

all: environ
environ: environ.o specifier.o env.o util.o

clean:
	${RM} environ *.o

.PHONY: clean install uninstall
