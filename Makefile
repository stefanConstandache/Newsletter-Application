CFLAGS = -Wall -g

all: build

build: server subscriber

server: server.cpp

subscriber: subscriber.cpp

clean:
	rm -f server subscriber
