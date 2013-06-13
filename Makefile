all: lazysig chatserver
lazysig:lazysig.c
  gcc -o lazysig -Wall lazysig.c
chatserver: chatserver.c
	gcc -std=c99 -o chatserver -Wall chatserver.c
clean: 
	rm chatserver lazysig
default: all


