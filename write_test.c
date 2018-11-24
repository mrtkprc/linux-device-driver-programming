#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define DEVICE "/dev/queue0"

int main(int argc, char **argv)
{
	int fd,res;
	char read_buf[500];
	fd = open(DEVICE,O_RDWR);
	
	if(fd == -1)
	{
		printf("file can not be opened\n");
		exit(-1);
	}
	printf("Please, enter message: ");
	scanf(" %[^\n]",read_buf);
	res = write(fd,read_buf,sizeof(read_buf));
	printf("Writing Data: %s\n",read_buf);
	return 0;
}

