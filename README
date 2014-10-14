You can modify and distribute this source code freely.   
Copyright (C) 2003 Li Suke,Software School of Peking University.

1. Compile the project
If you use Linux kernel 2.2.*, you should use make -f Makefile.20
to compile the project;
   [root@localhost]#make -f Makefile.20

if you use Linux kernel 2.4.*, just input make command.
   [root@localhost]#make.

2. load the module

First, creat the character devices:
   [root@localhost]#mknod /dev/ed_rec c 200 0
   [root@localhost]#mknod /dev/ed_tx c 201 0

then, let the ed_load and ed_unload can execute:
   [root@localhost]#chmod +x ed_unload
   [root@localhost]#chmod +x ed_load

last, load the driver:
   [root@localhost]#./ed_load
You can change the IP address in ed_load script file

3. run server
   [root@localhost]#./server

now you can use the ed0 device and send and receive data 
through it.:)

