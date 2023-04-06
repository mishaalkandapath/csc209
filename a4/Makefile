#port selection:
PORT=58137
CFLAGS= -DPORT=\$(PORT) -g -std=gnu99 -Wall -Werror

friend_server: friends.o friend_server.o
	gcc $(CFLAGS) -o friend_server friend_server.o friends.o

friend_server.o: friend_server.c
	gcc $(CFLAGS) -c friend_server.c

friends.o: friends.c friends.h
	gcc $(CFLAGS) -c friends.c

clean:
	rm friendme *.o