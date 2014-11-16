NetRouter
=========

Make pseudo network driver device a Data Transfer Stop based on SCULL under kernel 2.6.32 to realize serial access to internet.

FILE TREE:
ed_device.h:The description and struct of Linux Device Driver
ed_device.c:The main driver file to derive Linux Device Driver
server.c:   The server to listen to the coming of data packet
WriteData:  Test the function of server.c
LoadDevice: Load the driver into the kernel.
