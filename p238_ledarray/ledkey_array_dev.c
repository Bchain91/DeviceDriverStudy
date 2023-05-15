#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       
#include <linux/moduleparam.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>

#define   LED_DEV_NAME            "ledkeydev"
#define   LED_DEV_MAJOR            240      
#define DEBUG 1
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))

static unsigned long ledvalue = 15;
static char * twostring = NULL;

module_param(ledvalue, ulong ,0);
module_param(twostring,charp,0);

int led[4] = {
	IMX_GPIO_NR(1, 16),   //16
	IMX_GPIO_NR(1, 17),	  //17
	IMX_GPIO_NR(1, 18),   //18
	IMX_GPIO_NR(1, 19),   //19
};
int key[8] = {
	IMX_GPIO_NR(1, 20),   //20
	IMX_GPIO_NR(1, 21),	  //21
	IMX_GPIO_NR(4, 8),    //104
	IMX_GPIO_NR(4, 9),    //105
	IMX_GPIO_NR(4, 5),    //101
	IMX_GPIO_NR(7, 13),	  //205
	IMX_GPIO_NR(1, 7),    //7
	IMX_GPIO_NR(1, 8),    //8
};
static int ledkey_request(void)
{
	int ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(led); i++) {
		ret = gpio_request(led[i], "gpio led");
		if(ret<0){
			printk("#### FAILED Request gpio %d. error : %d \n", led[i], ret);
			break;
		} 
	}
	for (i = 0; i < ARRAY_SIZE(key); i++) {
		ret = gpio_request(key[i], "gpio key");
		if(ret<0){
			printk("#### FAILED Request gpio %d. error : %d \n", key[i], ret);
			break;
		} 
	}
	return ret;
}
static void ledkey_free(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(led); i++){
		gpio_free(led[i]);
	}
	for (i = 0; i < ARRAY_SIZE(key); i++){
		gpio_free(key[i]);
	}
}

void led_write(char* led_data)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(led); i++){
//		gpio_direction_output(led[i], (data >> i ) & 0x01);
//  	gpio_set_value(led[i], (data >> i ) & 0x01);
  		gpio_direction_output(led[i], 0);
    	gpio_set_value(led[i],led_data[i]);
#if DEBUG
	printk("#### %s, data = %d\n", __FUNCTION__, led_data[i]);
#endif
	}
}
void key_read(char* key_data)
{
	int i;
//	unsigned long data=0;
//	unsigned long temp;
	for(i=0;i<8;i++)
	{
  		gpio_direction_input(key[i]); //error led all turn off
		key_data[i] = gpio_get_value(key[i]);
#if DEBUG
	printk("#### %s, data = %d\n", __FUNCTION__, key_data[i]);
#endif
//		data |= temp;
// 		data  = data | temp;
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
	return;
}


int ledkeydev_open (struct inode *inode, struct file *filp)
{
    int num0 = MAJOR(inode->i_rdev); 
    int num1 = MINOR(inode->i_rdev); 
    printk( "ledkeydev open -> major : %d\n", num0 );
    printk( "ledkeydev open -> minor : %d\n", num1 );

    return 0;
}

loff_t ledkeydev_llseek (struct file *filp, loff_t off, int whence )
{
    printk( "ledkeydev llseek -> off : %08X, whenec : %08X\n", (unsigned int)off, whence );
    return 0x23;
}

ssize_t ledkeydev_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char kbuf[4];
	int i;
//	int ret;
//    printk( "ledkeydev read -> buf : %08X, count : %08X \n", (unsigned int)buf, count );
//	led_read(&kbuf);
	key_read(kbuf);     
	for(i = 0; i < count; i++)
	{
		put_user(kbuf[i],&buf[i]);
	}

//	ret=copy_to_user(buf,&kbuf,count);
//	if(ret < 0)
//		return -ENOMEM;
    return count;
}

ssize_t ledkeydev_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	char kbuf[4];
	int i;
//	int ret;
    printk( "ledkeydev write -> buf : %08X, count : %08X \n", (unsigned int)buf, count );
//	get_user(kbuf,buf);
	for(i=0; i<count; i++)
	{
		get_user(kbuf[i],buf);
//		get_user(*(kbuf+i),buf);
		buf++;
	}

//	ret=copy_from_user(kbuf,buf,count);
//	if(ret < 0)
//		return -ENOMEM;
	led_write(kbuf);
    return count;
}

//int ledkeydev_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
static long ledkeydev_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{

    printk( "ledkeydev ioctl -> cmd : %08X, arg : %08X \n", cmd, (unsigned int)arg );
    return 0x53;
}

int ledkeydev_release (struct inode *inode, struct file *filp)
{
    printk( "ledkeydev release \n" );
    return 0;
}

struct file_operations ledkeydev_fops =
{
    .owner    = THIS_MODULE,
    .open     = ledkeydev_open,     
    .read     = ledkeydev_read,     
    .write    = ledkeydev_write,    
	.unlocked_ioctl = ledkeydev_ioctl,
    .llseek   = ledkeydev_llseek,   
    .release  = ledkeydev_release,  
};

int ledkeydev_init(void)
{
    int result;

    printk( "ledkeydev ledkeydev_init \n" );    

    result = register_chrdev( LED_DEV_MAJOR, LED_DEV_NAME, &ledkeydev_fops);
    if (result < 0) return result;

	result = ledkey_request();
	if(result < 0)
	{
  		return result;     /* Device or resource busy */
	}
    return 0;
}

void ledkeydev_exit(void)
{
	char buf[4] = {0};
    printk( "ledkeydev ledkeydev_exit \n" );    
    unregister_chrdev( LED_DEV_MAJOR, LED_DEV_NAME );
	led_write(buf);
	ledkey_free();
}

module_init(ledkeydev_init);
module_exit(ledkeydev_exit);

MODULE_LICENSE("Dual BSD/GPL");
