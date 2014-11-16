#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

int main()
{
	int fd;
	int length;
	int num=0;
	unsigned char buff[]={192,30,40,50,60,70,10,20,219,10,20,221,12,192};
	length=sizeof(buff);
	fd=open("/dev/ttyUSB0",O_RDWR | O_NOCTTY);
	if(fd==-1)
	{
		perror("Open error\n");
		return -1;
	}
	num=write(fd,buff,length);
	printf("Have writen the data into SAC1,%d\n",num);
	close(fd);//注意一定要关闭fd描述符
}
