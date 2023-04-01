#port selection:
PORT=58137
CFLAGS= -DPORT=\$(PORT) -g -std=gnu99 -Wall -Werror

friend_server: friends.o friend_server.c
	gcc $(CFLAGS) -o friend_server friend_server.c

friendme: friendme.o friends.o
	gcc $(CFLAGS) -o friendme friendme.o friends.o

friendme.o: friendme.c friends.h
	gcc $(CFLAGS) -c friendme.c

friends.o: friends.c friends.h
	gcc $(CFLAGS) -c friends.c

clean:
	rm friendme *.o