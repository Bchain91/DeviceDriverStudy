#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>

#define DEVICE_LED_FILENAME "/proc/ledkey/led"
#define DEVICE_KEY_FILENAME "/proc/ledkey/key"

int main(int argc, char *argv[])
{
	int ledFd;
	int keyFd;
	char buff[10]={0};

	if(argc < 2)
	{
		printf("Usage : %s [0~15]\n",argv[0]);
		return 1;
	}

	ledFd = open(DEVICE_LED_FILENAME, O_WRONLY );
	if(ledFd < 0)
	{
		perror("open1");
		return 2;
	}
	keyFd = open(DEVICE_KEY_FILENAME, O_RDONLY );
	if(keyFd < 0)
	{
		perror("open2");
		return 3;
	}
	strcpy(buff,argv[1]);
	write(ledFd,buff,strlen(buff));
	printf("write : %s\n",buff);
	read(keyFd,buff,sizeof(buff));
	printf("read : %s\n",buff);
	close(ledFd);
	close(keyFd);
	return 0;
}
