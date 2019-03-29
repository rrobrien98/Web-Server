CC = gcc
CFLAGS = -Wall

serv: server.c
	$(CC) $(CFLAGS) -lpthread -o $@ server.c

clean:
	rm -f serv
