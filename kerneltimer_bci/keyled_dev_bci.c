#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/gpio.h>
#include <asm/io.h>
#include <asm/uaccess.h> 
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include "ioctl_timer.h"

#define LED_DEV_NAME "keyled"
#define LED_DEV_MAJOR 240

DECLARE_WAIT_QUEUE_HEAD(waitQueue_Read);

typedef struct
{
	struct timer_list timer;
	unsigned long     led;
	int				time_val;
} __attribute__ ((packed)) KERNEL_TIMER_MANAGER;


static int sw_irq[8] = {0};
static char sw_no = 0;

#define DEBUG 1
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))

void kerneltimer_timeover(unsigned long arg);
void led_write(unsigned long data);
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
		gpio_direction_output(led[i],0); 
	}
	for (i = 0; i < ARRAY_SIZE(key); i++) {
		sw_irq[i] = gpio_to_irq(key[i]);
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
		free_irq(sw_irq[i],NULL);
	}
}
void kerneltimer_registertimer(KERNEL_TIMER_MANAGER *pdata, unsigned long timeover)
{
	init_timer( &(pdata->timer) );
	pdata->timer.expires = get_jiffies_64() + timeover;  //10ms *100 = 1sec
	pdata->timer.data    = (unsigned long)pdata ;
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
		kerneltimer_registertimer( pdata, pdata->time_val);
	}
}

int kerneltimer_init(struct file* filp)
{
	KERNEL_TIMER_MANAGER* ptrmng = (KERNEL_TIMER_MANAGER*)filp->private_data;
	kerneltimer_registertimer( ptrmng, ptrmng->time_val);
	return 0;
}
void kerneltimer_exit(struct file* filp)
{
	KERNEL_TIMER_MANAGER* ptrmng = (KERNEL_TIMER_MANAGER*)filp->private_data;
	del_timer(&(ptrmng->timer));
}


void led_write(unsigned long data)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(led); i++){
		gpio_set_value(led[i], (data >> i) & 0x01);
	}
#if DEBUG
	printk("#### %s, data = %ld\n", __FUNCTION__, data);
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
	KERNEL_TIMER_MANAGER* ptrmng;
	int num = MINOR(inode->i_rdev);
	
	ptrmng = kmalloc(sizeof(KERNEL_TIMER_MANAGER),GFP_KERNEL);
	if(ptrmng == NULL) return -ENOMEM;
	printk("ledkeydev open -> minor : %d\n", num);
	num = MAJOR(inode->i_rdev);
	printk("ledkeydev open -> major : %d\n", num);
	memset(ptrmng,0x00,sizeof(KERNEL_TIMER_MANAGER));
	filp->private_data = (void*)ptrmng;
	kerneltimer_registertimer(ptrmng,ptrmng->time_val);
	return 0;
}
static ssize_t ledkeydev_read(struct file *filp,char *buf, size_t count, loff_t *f_pos)
{
	int ret;
#if DEBUG
	printk("ledkeydev read -> buf : %08X, count : %08X\n",(unsigned int)buf,count);
#endif

	if(!(filp->f_flags & O_NONBLOCK))	//O_BLOCK Mode
	{
		if(sw_no == 0)
			interruptible_sleep_on(&waitQueue_Read);
		//		wait_event_interruptible_timeout(waitQueue_Read,sw_no,100);	//100: 100hz * 100 = 1sec
		//		wait_event_interruptible(waitQueue_Read,sw_no);		
	}
	ret = copy_to_user(buf,&sw_no,count);
	sw_no = 0;
	if(ret < 0)
		return -ENOMEM;
	return count;
}
static ssize_t ledkeydev_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	KERNEL_TIMER_MANAGER* ptrmng = (KERNEL_TIMER_MANAGER*)filp->private_data;
	int ret;
#if DEBUG
	printk("ledkeydev write -> buf : %08X, count : %08X\n",(unsigned int)buf,count);
#endif
	ret = copy_from_user(&ptrmng->led,buf,count);
	if(ret<0)
		return -ENOMEM;	
//	led_write(ptrmng->led);
	return count;
}
static long ledkeydev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	keyled_data ctrl_Info = {0};
	KERNEL_TIMER_MANAGER* ptrmng = (KERNEL_TIMER_MANAGER*)filp->private_data;
	int err, size;

	if( _IOC_TYPE( cmd ) != TIMER_MAGIC ) return -EINVAL;
	if( _IOC_NR( cmd ) >= TIMER_MAXNR ) return -EINVAL;

	size = _IOC_SIZE( cmd );
	if( size )
	{
		err = 0;
		if( _IOC_DIR( cmd ) & _IOC_READ )
			err = access_ok( VERIFY_WRITE, (void *) arg, size );
		if( _IOC_DIR( cmd ) & _IOC_WRITE )
			err = access_ok( VERIFY_READ , (void *) arg, size );
		if( !err ) return err;
	}
	switch( cmd )
	{
		case TIMER_STOP :
			if(timer_pending(&(ptrmng->timer)))
				kerneltimer_exit(filp);
			break;
		case TIMER_START :
			if(!timer_pending(&(ptrmng->timer)))
				kerneltimer_init(filp);
			break;
		case TIMER_VALUE :
			err = copy_from_user(&ctrl_Info,(void*)arg,size);
			ptrmng->time_val = ctrl_Info.timer_val;
			break;
		default:
			err =-E2BIG;
			break;
	}
#if DEBUG
	printk("ledkeydev ioctl -> cmd : %08X, arg : %d\n",cmd,ptrmng->time_val);
#endif
	return err;

}
static unsigned int ledkeydev_poll (struct file *filp, struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	printk("_key : %ld \n",(wait->_key & POLLIN));
	if(wait->_key & POLLIN)
		poll_wait(filp, &waitQueue_Read, wait);
	if(sw_no > 0)
		mask = POLLIN;
	return mask;
}
static int ledkeydev_release(struct inode *inode, struct file *filp)
{
	printk("ledkeydev release \n");
	led_write(0);
	ledkey_free();
	kerneltimer_exit(filp);	
	if(filp->private_data)
		kfree(filp->private_data);
	filp->private_data = NULL;
	return 0;
}
struct file_operations ledkeydev_fops =
{
	.owner			= THIS_MODULE,
	.open			= ledkeydev_open,
	.read			= ledkeydev_read,
	.write			= ledkeydev_write,
	.unlocked_ioctl	= ledkeydev_ioctl,
	.poll			= ledkeydev_poll,
	.release		= ledkeydev_release,
};

irqreturn_t sw_isr(int irq, void *unuse)
{
	int i;
	for(i=0; i<ARRAY_SIZE(key); i++)
	{
		if(irq == sw_irq[i])
		{
			sw_no = i+1;
			break;
		}
	}
#if DEBUG
	printk("IRQ : %d, sw_no : %d\n", irq,sw_no);
#endif
	wake_up_interruptible(&waitQueue_Read);
	return IRQ_HANDLED;
}
static int ledkeydev_init(void)
{
	int result;
	int i;
	char* sw_name[8] = {"key1","key2","key3","key4","key5","key6","key7","key8"};
	printk("ledkeydev ledkeydev_init \n");
	result = register_chrdev(LED_DEV_MAJOR,LED_DEV_NAME, &ledkeydev_fops);
	if(result < 0) return result;
	result = ledkey_request();
	if(result<0)
	{
		return result; 
	}
	for(i=0; i<ARRAY_SIZE(key); i++)
	{
		result = request_irq(sw_irq[i],sw_isr,IRQF_TRIGGER_RISING,sw_name[i],NULL);
		if(result<0)
		{
			printk("#### FAILED Request irq : %d. error : %d \n", sw_irq[i], result);
			break;
		}
	}
	return result;
}
static void ledkeydev_exit(void)
{
	printk("ledkeydev ledkeydev_exit \n");
	led_write(0);
	unregister_chrdev(LED_DEV_MAJOR,LED_DEV_NAME);
}
module_init(ledkeydev_init);
module_exit(ledkeydev_exit);
MODULE_LICENSE("Dual BSD/GPL");
