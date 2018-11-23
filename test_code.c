#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define DEVICE "/dev/queue0"

int main(int argc, char **argv)
{
	int fd,res;
	char read_buf[200];
	char write_buf[] = "OmerRecep";
	fd = open(DEVICE,O_RDWR);
	
	if(fd == -1)
	{
		printf("file can not be opened\n");
		exit(-1);
	}
	
	read(fd,read_buf,sizeof(read_buf));
	//res = write(fd,write_buf,sizeof(write_buf));
	printf("Reading Data: %s\n",read_buf);
	//printf("Writing Data: %d\n",res);
	return 0;
}

