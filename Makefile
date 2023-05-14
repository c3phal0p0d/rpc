CC=cc
RPC_SYSTEM=rpc.o

.PHONY: format all

all: $(RPC_SYSTEM) rpc-client rpc-server

$(RPC_SYSTEM): rpc.c rpc.h
	$(CC) -c -Wall -g -o $@ $<

rpc-client: client.c rpc.o
	cc -Wall -g -o rpc-client client.c rpc.o

rpc-server: server.c rpc.o
	cc -Wall -g -o rpc-server server.c rpc.o

# utils.o: utils.c utils.h
# 	$(CC) -c -Wall -o $@ $<

RPC_SYSTEM_A=rpc.a
$(RPC_SYSTEM_A): rpc.o
	ar rcs $(RPC_SYSTEM_A) $(RPC_SYSTEM)

format:
	clang-format -style=file -i *.c *.h

clean:
	rm -f *.o rpc-client rpc-server
