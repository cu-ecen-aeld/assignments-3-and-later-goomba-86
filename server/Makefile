CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -O2
DFLAGS ?= -Wall -Wextra -Werror -O0 -g

all:
	$(CC) $(CFLAGS) aesdsocket.c -o aesdsocket

debug:
	$(CC) $(DFLAGS) aesdsocket.c -o aesdsocket

default:
	$(CC) $(CFLAGS) aesdsocket.c -o aesdsocket

clean:
	rm -f aesdsocket
	rm -f *.o
