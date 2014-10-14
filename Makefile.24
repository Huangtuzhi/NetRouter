CC=gcc
CFLAGS= -Wall -DMODULE -D__KERNEL__ -DLINUX  -DLINUX_24 -I/usr/src/linux-2.4/include
all:ed_device.o server
ed_device.o:ed_device.c
server:server.c
	gcc  server.c -o server
clean:
	rm *.o
	rm ./server
