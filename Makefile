client: client.cpp
	g++ -Wall -Wextra -g client.cpp -o client

server: server.cpp
	g++ -Wall -Wextra -g server.cpp -o server

all: client server
