CC=cc
RPC_SYSTEM=rpc.o
RPC_SYSTEM_A=rpc.a

.PHONY: format all

all: $(RPC_SYSTEM) $(RPC_SYSTEM_A) rpc-client rpc-server test-client test-server

$(RPC_SYSTEM): rpc.c rpc.h
	$(CC) -c -Wall -g -o $@ $<

rpc-client: client.c rpc.o
	$(CC) -Wall -g -o rpc-client client.c rpc.o

rpc-server: server.c rpc.o
	$(CC) -Wall -g -o rpc-server server.c rpc.o

$(RPC_SYSTEM_A): rpc.o
	ar rcs $(RPC_SYSTEM_A) $(RPC_SYSTEM)

# test-client: 
# 	$(CC) -o test-client artifacts/client.a rpc.a

# test-server:
# 	$(CC) -o test-server artifacts/server.a rpc.a

format:
	clang-format -style=file -i *.c *.h

clean:
	rm -f *.o *.a rpc-client rpc-server test-client test-server
