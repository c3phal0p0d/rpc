CC=cc
RPC_SYSTEM=rpc.o
RPC_SYSTEM_A=rpc.a

.PHONY: format all

all: $(RPC_SYSTEM) $(RPC_SYSTEM_A)

$(RPC_SYSTEM): rpc.c rpc.h
	$(CC) -c -Wall -g -o $@ $<

$(RPC_SYSTEM_A): rpc.o
	ar rcs $(RPC_SYSTEM_A) $(RPC_SYSTEM)

format:
	clang-format -style=file -i *.c *.h

clean:
	rm -f *.o *.a
