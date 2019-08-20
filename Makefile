main: client/client.c server/server.c
	gcc -g client/client.c -o client/client
	gcc -g server/server.c -o server/server

clean:
	rm client/client
	rm server/server
