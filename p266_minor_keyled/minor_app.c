#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define WRITE_DEVICE_FILENAME  "/dev/minor_write" //Major 240, Minor 1
#define READ_DEVICE_FILENAME   "/dev/minor_read"  //Major 240, Minor 2

int main()
{
    int  read_dev;
    int  write_dev;
	int  ret;
	int  key_old=0;
    
    char buff[128]={0};
    int loop;

    read_dev  = open( READ_DEVICE_FILENAME,  O_RDWR|O_NDELAY );
    if( read_dev < 0 ) 
    {  
        printf( READ_DEVICE_FILENAME "open error\n" );
		perror("read_dev open()");
        exit(1);
    }
    
    write_dev = open( WRITE_DEVICE_FILENAME, O_RDWR|O_NDELAY );
    if( write_dev < 0 ) 
    {  
        printf( WRITE_DEVICE_FILENAME "open error\n" );
		perror("write_dev open");
        close( read_dev );
        exit(1);
    }
    
    printf( "wait... input\n" );
    while(1)
    {
        ret =  read(read_dev,buff,1 );
        if(buff[0] == 0 || buff[0] == key_old )
			continue;
		key_old = buff[0];
        printf( "read data [%02X]\n", buff[0] & 0xFF );
        if(buff[0] & 0x01) break;
    } 
    printf( "input ok...\n");
    
    printf( "led flashing...\n");
    for( loop=0; loop<5; loop++ )
    {
        buff[0] = 0x0F; 
        write(write_dev,buff,1 );
        sleep(1);
        buff[0] = 0x00; 
        write(write_dev,buff,1 );
        sleep(1);
    }
        
    close(read_dev);
    close(write_dev);

    return 0;
}

