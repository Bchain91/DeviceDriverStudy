#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>
#include <asm/uaccess.h> 
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <linux/wait.h>
#include <linux/string.h>

#define LED_DEV_NAME "ledkeydev"
#define LED_DEV_MAJOR 240

DECLARE_WAIT_QUEUE_HEAD(waitQueue_Read);

static int irq_init(struct file *filp);

static unsigned long ledvalue = 15;

module_param(ledvalue, ulong , 0);

typedef struct
{
	int sw_irq[8];
	char sw_no;
} __attribute__ ((packed)) ISR_INFO;

#define DEBUG 1
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))

int led[4] = {
	IMX_GPIO_NR(1, 16),	//16
	IMX_GPIO_NR(1, 17),	//17
	IMX_GPIO_NR(1, 18),	//18
	IMX_GPIO_NR(1, 19),	//19
};
int key[8] = {
	IMX_GPIO_NR(1, 20),	//20
	IMX_GPIO_NR(1, 21),	//21
	IMX_GPIO_NR(4, 8),	//104
	IMX_GPIO_NR(4, 9),	//105
	IMX_GPIO_NR(4, 5),	//101
	IMX_GPIO_NR(7, 13),	//205
	IMX_GPIO_NR(1, 7),	//7
	IMX_GPIO_NR(1, 8),	//8
};
static int ledkey_request(struct file * filp)
{
	ISR_INFO* pIsrInfo = (ISR_INFO*)filp->private_data;
	int ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(led); i++) {
		ret = gpio_request(led[i], "gpio led");
		if(ret<0){
			printk("#### FAILED Request gpio %d. error : %d \n", led[i], ret);
			break;
		} 
		gpio_direction_output(led[i],0); 
	}
	for (i = 0; i < ARRAY_SIZE(key); i++) {
		pIsrInfo->sw_irq[i] = gpio_to_irq(key[i]);
		if(ret<0){
			printk("#### FAILED Request gpio %d. error : %d \n", key[i], ret);
			break;
		} 
	}
	return ret;
}

static void ledkey_free(struct file * filp)
{
	ISR_INFO* pIsrInfo = (ISR_INFO*)filp->private_data;
	int i;
	for (i = 0; i < ARRAY_SIZE(led); i++){
		gpio_free(led[i]);
	}
	for (i = 0; i < ARRAY_SIZE(key); i++){
		//		gpio_free(key[i]);
		free_irq(pIsrInfo->sw_irq[i],filp->private_data);
	}
}

void led_write(unsigned char data)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(led); i++){
		gpio_set_value(led[i], (data >> i) & 0x01);
	}
#if DEBUG
	printk("#### %s, data = %d\n", __FUNCTION__, data);
#endif
}

void key_read(unsigned char* key_data)
{
	int i;
	unsigned long data=0;
	unsigned long temp;
	for(i=0;i<ARRAY_SIZE(key);i++)
	{
		temp = gpio_get_value(key[i]);
		if(temp){
			data = 1+i;
			break;
		}
	}
#if DEBUG
	printk("#### %s, data = %ld\n", __FUNCTION__, data);
#endif
	*key_data = data;
	return;
}

static int ledkeydev_open(struct inode *inode, struct file *filp)
{
	ISR_INFO* pIsrInfo;
	int num = MINOR(inode->i_rdev);

	pIsrInfo = kmalloc(sizeof(ISR_INFO),GFP_KERNEL);
	printk("ledkeydev open -> minor : %d\n", num);
	num = MAJOR(inode->i_rdev);
	printk("ledkeydev open -> major : %d\n", num);
//	pIsrInfo->sw_no = 0;
	memset(pIsrInfo,0x00,sizeof(ISR_INFO));
	filp->private_data = (void*)pIsrInfo;
	irq_init(filp);

	return 0;
}
static ssize_t ledkeydev_read(struct file *filp,char *buf, size_t count, loff_t *f_pos)
{
	int ret;
	ISR_INFO* pIsrInfo = (ISR_INFO*)filp->private_data;
#if DEBUG
	printk("ledkeydev read -> buf : %08X, count : %08X\n",(unsigned int)buf,count);
#endif
	if(!(filp->f_flags & O_NONBLOCK))	//O_BLOCK Mode
	{
		if(pIsrInfo->sw_no == 0)
			interruptible_sleep_on(&waitQueue_Read);
	}
	ret = copy_to_user(buf,&pIsrInfo->sw_no,count);
	pIsrInfo->sw_no = 0;
	if(ret < 0)
		return -ENOMEM;
	return count;
}
static ssize_t ledkeydev_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	char kbuf;
	int ret;
#if DEBUG
	printk("ledkeydev write -> buf : %08X, count : %08X\n",(unsigned int)buf,count);
#endif
	ret = copy_from_user(&kbuf,buf,count);
	if(ret<0)
		return -ENOMEM;	
	led_write(kbuf);
	return count;
}
static long ledkeydev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
#if DEBUG
	printk("ledkeydev ioctl -> cmd : %08X, arg : %08X\n",cmd,(unsigned int)arg);
#endif
	return 0x53;
}
		//		gpio_direction_input(key[i]); //error led all turn off
static int ledkeydev_release(struct inode *inode, struct file *filp)
{
	printk("ledkeydev release \n");
	led_write(0);
	ledkey_free(filp);
	if(filp->private_data)
		kfree(filp->private_data);
	return 0;
}
struct file_operations ledkeydev_fops =
{
	.owner	= THIS_MODULE,
	.open	= ledkeydev_open,
	.read	= ledkeydev_read,
	.write	= ledkeydev_write,
	.unlocked_ioctl	= ledkeydev_ioctl,
	.release	= ledkeydev_release,
};

irqreturn_t sw_isr(int irq, void *private_data)
{
	int i;
	ISR_INFO* pIsrInfo = (ISR_INFO*)private_data;
	for(i=0; i<ARRAY_SIZE(key); i++)
	{
		if(irq == pIsrInfo->sw_irq[i])
		{
			pIsrInfo->sw_no= i+1;
			break;
		}
	}
#if DEBUG
	printk("IRQ : %d, sw_no : %d\n", irq, pIsrInfo->sw_no);
#endif
	wake_up_interruptible(&waitQueue_Read);
	return IRQ_HANDLED;
}
static int irq_init(struct file *filp)
{
	int result;
	int i;
	char* sw_name[8] = {"key1","key2","key3","key4","key5","key6","key7","key8"};
	ISR_INFO* pIsrInfo = (ISR_INFO*)filp->private_data;
	result = ledkey_request(filp);
	if(result<0)
	{
		return result; 
	}
	for(i=0; i<ARRAY_SIZE(key); i++)
	{
		result = request_irq(pIsrInfo->sw_irq[i],sw_isr,IRQF_TRIGGER_RISING,sw_name[i],filp->private_data);
		if(result)
		{
			printk("#### FAILED Request irq : %d. error : %d \n",pIsrInfo->sw_irq[i], result);
			break;
		}
	}
	return result;
}
static int ledkeydev_init(void)
{
	int result;
	printk("ledkeydev ledkeydev_init \n");
	result = register_chrdev(LED_DEV_MAJOR,LED_DEV_NAME, &ledkeydev_fops);
	if(result < 0) return result;
	return result;
}
static void ledkeydev_exit(void)
{
	printk("ledkeydev ledkeydev_exit \n");
	unregister_chrdev(LED_DEV_MAJOR,LED_DEV_NAME);
}
module_init(ledkeydev_init);
module_exit(ledkeydev_exit);
MODULE_LICENSE("Dual BSD/GPL");
