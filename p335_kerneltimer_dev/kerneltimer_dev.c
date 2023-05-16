#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>

#define LED_DEV_NAME "kerneltimer"
#define LED_DEV_MAJOR 240

#define TIME_STEP	timeval  //KERNEL HZ=100
#define DEBUG 1
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))
static int timeval = 100;	//f=100HZ, T=1/100 = 10ms, 100*10ms = 1Sec
module_param(timeval,int ,0);
static int ledval = 0;
module_param(ledval,int ,0);
typedef struct
{
	struct timer_list timer;
	unsigned long 	  led;
} __attribute__ ((packed)) KERNEL_TIMER_MANAGER;

static KERNEL_TIMER_MANAGER* ptrmng = NULL;
void kerneltimer_timeover(unsigned long arg);

int key[] = {
	IMX_GPIO_NR(1, 20),
	IMX_GPIO_NR(1, 21),
	IMX_GPIO_NR(4, 8),
	IMX_GPIO_NR(4, 9),
	IMX_GPIO_NR(4, 5),
	IMX_GPIO_NR(7, 13),
	IMX_GPIO_NR(1, 7),
	IMX_GPIO_NR(1, 8),
};

int led[] = {
	IMX_GPIO_NR(1, 16),
	IMX_GPIO_NR(1, 17),
	IMX_GPIO_NR(1, 18),
	IMX_GPIO_NR(1, 19),
};

void led_write(char data)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(led); i++){
		gpio_set_value(led[i], ((data >> i) & 0x01));
#if DEBUG
		printk("#### %s, data = %d(%d)\n", __FUNCTION__, data,(data>>i));
#endif
	}
}
void kerneltimer_registertimer(KERNEL_TIMER_MANAGER *pdata, unsigned long timeover)
{
	init_timer( &(pdata->timer) );
	pdata->timer.expires = get_jiffies_64() + timeover;  //10ms *100 = 1sec
	pdata->timer.data	 = (unsigned long)pdata ;
	pdata->timer.function = kerneltimer_timeover;
	add_timer( &(pdata->timer) );
}
void kerneltimer_timeover(unsigned long arg)
{
	KERNEL_TIMER_MANAGER* pdata = NULL;
	if( arg )
	{
		pdata = ( KERNEL_TIMER_MANAGER *)arg;
		led_write(pdata->led & 0x0f);
#if DEBUG
		printk("led : %#04x\n",(unsigned int)(pdata->led & 0x0000000f));
#endif
		pdata->led = ~(pdata->led);
		kerneltimer_registertimer( pdata, TIME_STEP);
	}
}

int kerneltimer_init(void)
{
	printk("timeval : %d , sec : %d , size : %d\n",timeval,timeval/HZ, sizeof(KERNEL_TIMER_MANAGER ));

	ptrmng = (KERNEL_TIMER_MANAGER *)kmalloc( sizeof(KERNEL_TIMER_MANAGER ), GFP_KERNEL);
	if(ptrmng == NULL) return -ENOMEM;
	memset( ptrmng, 0, sizeof( KERNEL_TIMER_MANAGER));
	ptrmng->led = ledval;
	kerneltimer_registertimer( ptrmng, TIME_STEP);
	return 0;
}
void kerneltimer_exit(void)
{
	if(timer_pending(&(ptrmng->timer)))
		del_timer(&(ptrmng->timer));
	if(ptrmng != NULL)
	{
		kfree(ptrmng);
	}
	led_write(0);
}
static void key_read(char * key_data)
{
	int i;
	char data=0;
	for(i=0;i<ARRAY_SIZE(key);i++)
	{
		if(gpio_get_value(key[i]))
		{
			data = i+1;
			break;
		}
	}
#if DEBUG
	printk("#### %s, data = %d\n", __FUNCTION__, data);
#endif
	*key_data = data;
	return;
}

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

static int ledkeydev_open(struct inode *inode, struct file *filp)
{
	int num = MINOR(inode->i_rdev);
	printk("ledkeydev open -> minor : %d\n", num);
	num = MAJOR(inode->i_rdev);
	printk("ledkeydev open -> major : %d\n", num);
	return 0;
}
static ssize_t ledkeydev_read(struct file *filp,char *buf, size_t count, loff_t *f_pos)
{
	char kbuf;
	int ret;
	printk("ledkeydev read -> buf : %08X, count : %08X\n",(unsigned int)buf,count);
	//  led_read(&kbuf);
	key_read(&kbuf);
	//  put_user(kbuf,buf);
	ret = copy_to_user(buf,&kbuf,count);
	if(ret < 0)
		return -ENOMEM;
	return count;
}
static ssize_t ledkeydev_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	char kbuf;
	int ret;
	printk("ledkeydev write -> buf : %08X, count : %08X\n",(unsigned int)buf,count);
	//  get_user(kbuf,buf);
	ret = copy_from_user(&kbuf,buf,count);
	if(ret<0)
		return -ENOMEM;
	ptrmng->led = kbuf;
	return count;
}
static int ledkeydev_release(struct inode *inode, struct file *filp)
{
	printk("ledkeydev release \n");
	return 0;
}

struct file_operations ledkeydev_fops =
{
	.owner  = THIS_MODULE,
	.open   = ledkeydev_open,
	.read   = ledkeydev_read,
	.write  = ledkeydev_write,
	.release    = ledkeydev_release,
};
static int ledkeydev_init(void)
{
	int result;
	printk("ledkeydev ledkeydev_init \n");
	result = register_chrdev(LED_DEV_MAJOR,LED_DEV_NAME, &ledkeydev_fops);
	if(result < 0) return result;
	result = ledkey_request();
	if(result<0)
	{
		return result;
	}
	kerneltimer_init();
	return 0;
}
static void ledkeydev_exit(void)
{
	printk("ledkeydev ledkeydev_exit \n");
	unregister_chrdev(LED_DEV_MAJOR,LED_DEV_NAME);
	ledkey_free();
	kerneltimer_exit();
}
module_init(ledkeydev_init);
module_exit(ledkeydev_exit);
MODULE_LICENSE("Dual BSD/GPL");
