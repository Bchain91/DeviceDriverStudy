#ifndef __IOCTL_H__
#define __IOCTL_H__

#define TIMER_MAGIC 't'
typedef struct{
	unsigned long timer_val;
}__attribute__((packed)) keyled_data;

typedef struct
{
	unsigned long timer_val;
	unsigned long size;
	unsigned char buff[128];
} __attribute__((packed)) ioctl_test_info;

#define TIMER_STOP 	_IO(TIMER_MAGIC, 0) 
#define TIMER_START	_IO(TIMER_MAGIC, 1)
#define TIMER_VALUE	_IOWR(TIMER_MAGIC, 2,keyled_data)
#define TIMER_MAXNR			8
#endif
