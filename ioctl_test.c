#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "queue_ioctl.h"

#define DEVICE "/dev/queue0"

int main(int argc, char **argv)
{
	int fd,res;
	char write_buf[500];
	fd = open(DEVICE,O_RDWR);
	
	if(fd == -1)
	{
		printf("file can not be opened\n");
		exit(-1);
	}
	
	res = ioctl(fd,QUEUE_POP,write_buf);
	printf("IOCTL String and res: %s",write_buf);//,strerror(res));
	return 0;
}

