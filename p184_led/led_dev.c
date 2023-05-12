#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>

#define CALL_DEV_NAME "leddev"
#define CALL_DEV_MAJOR 240

static unsigned long ledvalue = 15;

module_param(ledvalue, ulong , 0);

#define DEBUG 1
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))

int led[] = {
	IMX_GPIO_NR(1, 16),   //16
	IMX_GPIO_NR(1, 17),	  //17
	IMX_GPIO_NR(1, 18),   //18
	IMX_GPIO_NR(1, 19),   //19
};
static int led_request(void)
{
	int ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(led); i++) {
		ret = gpio_request(led[i], "gpio led");
		if(ret<0){
			printk("#### FAILED Request gpio %d. error : %d \n", led[i], ret);
		} 
	}
	return ret;
}

static void led_free(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(led); i++){
		gpio_free(led[i]);
	}
}

void led_write(unsigned long data)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(led); i++){
		gpio_direction_output(led[i], (data >> i ) & 0x01);
//		gpio_set_value(led[i], (data >> i ) & 0x01);
	}
#if DEBUG
	printk("#### %s, data = %ld\n", __FUNCTION__, data);
#endif
}

void led_read(char* led_data)
{
	int i;
	unsigned long data=0;
	unsigned long temp;
	for(i=0;i<4;i++)
	{
  		gpio_direction_input(led[i]); //error led all turn off
		temp = gpio_get_value(led[i]) << i;
		data |= temp;
	}
/*	
	for(i=3;i>=0;i--)
	{
  		gpio_direction_input(led[i]); //error led all turn off
		temp = gpio_get_value(led[i]);
		data |= temp;
		if(i==0)
			break;
		data <<= 1;  //data <<= 1;
	}
*/
#if DEBUG
	printk("#### %s, data = %ld\n", __FUNCTION__, data);
#endif
	*led_data = data;
	led_write(data);
	return;
}

static int leddev_open(struct inode *inode, struct file *filp)
{
	int num = MINOR(inode->i_rdev);
	printk("leddev open -> minor : %d\n", num);
	num = MAJOR(inode->i_rdev);
	printk("leddev open -> major : %d\n", num);
	return 0;
}
static loff_t leddev_llseek(struct file *filp, loff_t off, int whence)
{
	printk("leddev llseek -> off : %08X, whence : %08X\n",(unsigned int)off,whence);
	return 0x23;
}
static ssize_t leddev_read(struct file *filp,char *buf, size_t count, loff_t *f_pos)
{
	printk("leddev read -> buf : %08X, count : %08X\n",(unsigned int)buf,count);
	led_read(buf);
	return count;
}
static ssize_t leddev_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	printk("leddev write -> buf : %08X, count : %08X\n",(unsigned int)buf,count);
	led_write(*buf);
	return 0x43;
}
static long leddev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	printk("leddev ioctl -> cmd : %08X, arg : %08X\n",cmd,(unsigned int)arg);
	return 0x53;
}
static int leddev_release(struct inode *inode, struct file *filp)
{
	printk("leddev release \n");
	return 0;
}
struct file_operations leddev_fops =
{
	.owner	= THIS_MODULE,
	.open	= leddev_open,
	.read	= leddev_read,
	.write	= leddev_write,
	.unlocked_ioctl	= leddev_ioctl,
	.llseek	= leddev_llseek,
	.release	= leddev_release,
};
static int leddev_init(void)
{
	int result;
	printk("leddev leddev_init \n");
	result = register_chrdev(CALL_DEV_MAJOR,CALL_DEV_NAME, &leddev_fops);
	if(result < 0) return result;
	result = led_request();
	if(result<0)
	{
		return result; 
	}
	return 0;
}
static void leddev_exit(void)
{
	printk("leddev leddev_exit \n");
	unregister_chrdev(CALL_DEV_MAJOR,CALL_DEV_NAME);
	led_write(0);
	led_free();
}
module_init(leddev_init);
module_exit(leddev_exit);
MODULE_LICENSE("Dual BSD/GPL");
