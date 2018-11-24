#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

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
	
	read(fd,write_buf,sizeof(write_buf));
	printf("Reading Data: %s\n",write_buf);
	return 0;
}

