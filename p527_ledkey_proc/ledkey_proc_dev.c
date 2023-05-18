#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>

#define   LEDKEY_DEV_NAME            "ledkey_proc"
#define   LEDKEY_DEV_MAJOR            240      
#define DEBUG 0
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))

DECLARE_WAIT_QUEUE_HEAD(WaitQueue_Read);
static struct proc_dir_entry *keyledproc_root_fp   = NULL;
static struct proc_dir_entry *keyledproc_led_fp    = NULL;
static struct proc_dir_entry *keyledproc_key_fp    = NULL;
static int sw_irq[8] = {0};
static long sw_no = 0;
static int led[] = {
	IMX_GPIO_NR(1, 16),   //16
	IMX_GPIO_NR(1, 17),	  //17
	IMX_GPIO_NR(1, 18),   //18
	IMX_GPIO_NR(1, 19),   //19
};

static int key[] = {
	IMX_GPIO_NR(1, 20),
	IMX_GPIO_NR(1, 21),
	IMX_GPIO_NR(4, 8),
	IMX_GPIO_NR(4, 9),
  	IMX_GPIO_NR(4, 5),
  	IMX_GPIO_NR(7, 13),
  	IMX_GPIO_NR(1, 7),
 	IMX_GPIO_NR(1, 8),
};
static int led_init(void)
{
	int ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(led); i++) {
		ret = gpio_request(led[i], "gpio led");
		if(ret<0){
			printk("#### FAILED Request gpio %d. error : %d \n", led[i], ret);
		} 
		else
			gpio_direction_output(led[i], 0);  //0:led off
	}
	return ret;
}
#if 0
static int key_init(void)
{
	int ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(key); i++) {
		ret = gpio_request(key[i], "gpio key");
		if(ret<0){
			printk("#### FAILED Request gpio %d. error : %d \n", key[i], ret);
		} 
		else
			gpio_direction_input(key[i]);  
	}
	return ret;
}
#endif
irqreturn_t sw_isr(int irq, void *unuse)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(key); i++) {
		if(irq == sw_irq[i]) {
			sw_no = i+1;
			break;
		}
	}
	printk("IRQ : %d, %ld\n",irq, sw_no);
  	wake_up_interruptible(&WaitQueue_Read);
	return IRQ_HANDLED;
}
static int key_irq_init(void)
{
	int ret=0;
	int i;
	char * irq_name[8] = {"irq sw1","irq sw2","irq sw3","irq sw4","irq sw5","irq sw6","irq sw7","irq sw8"};

	for (i = 0; i < ARRAY_SIZE(key); i++) {
		sw_irq[i] = gpio_to_irq(key[i]);
	}
	for (i = 0; i < ARRAY_SIZE(key); i++) {
		ret = request_irq(sw_irq[i],sw_isr, IRQF_TRIGGER_RISING, irq_name[i],NULL);
		if(ret) {
			printk("### FAILED Request irq %d. error : %d\n",sw_irq[i],ret);
		}
	}
	return ret;
}
static void led_exit(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(led); i++){
		gpio_free(led[i]);
	}
}
#if 0
static void key_exit(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(key); i++){
		gpio_free(key[i]);
	}
}
#endif
static void key_irq_exit(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(key); i++){
		free_irq(sw_irq[i],NULL);
	}
}
static void led_write(char data)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(led); i++){
		gpio_set_value(led[i], (data >> i ) & 0x01);
	}
#if DEBUG
	printk("#### %s, data = %d\n", __FUNCTION__, data);
#endif
}
#if 0
static void key_read(char * key_data)
{
	int i;
	char data=0;
//  	char temp;
	for(i=0;i<ARRAY_SIZE(key);i++)
	{
  		if(gpio_get_value(key[i]))
		{
			data = i+1;
			break;
		}
  
//  		temp = gpio_get_value(key[i]) << i;
//  		data |= temp;
	}
#if DEBUG
	printk("#### %s, data = %d\n", __FUNCTION__, data);
#endif
	*key_data = data;
	return;
}
#endif
static int ledkey_open (struct inode *inode, struct file *filp)
{
    int num0 = MAJOR(inode->i_rdev); 
    int num1 = MINOR(inode->i_rdev); 
    printk( "call open -> major : %d\n", num0 );
    printk( "call open -> minor : %d\n", num1 );

    return 0;
}

static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char kbuf;
	int ret;
	if(!(filp->f_flags & O_NONBLOCK))
	{
    	if(sw_no == 0)	
    		interruptible_sleep_on(&WaitQueue_Read);
//  		wait_event_interruptible(WaitQueue_Read,sw_no);
//    		wait_event_interruptible_timeout(WaitQueue_Read,sw_no,100);	//100 * 1/100H = 1Sec
//			if(sw_no == 0)
//				return 0;
	}
	kbuf = (char)sw_no;
	ret=copy_to_user(buf,&kbuf,count);
	sw_no = 0;
    return count;
}

static ssize_t ledkey_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
//    printk( "call write -> buf : %08X, count : %08X \n", (unsigned int)buf, count );
	char kbuf;
	int ret;
//	get_user(kbuf,buf);
	ret=copy_from_user(&kbuf,buf,count);
	led_write(kbuf);
    return count;
//	return -EFAULT;
}

static long ledkey_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{

    printk( "call ioctl -> cmd : %08X, arg : %08X \n", cmd, (unsigned int)arg );
    return 0x53;
}
static unsigned int ledkey_poll(struct file *filp, struct poll_table_struct *wait)
{
	int mask=0;
  	printk("_key : %ld \n",(wait->_key & POLLIN));
//	printk("_key : %ld \n",(wait->_key & POLLOUT));
	if(wait->_key & POLLIN)	
		poll_wait(filp, &WaitQueue_Read, wait);
	if(sw_no > 0)
		mask = POLLIN;
	return mask;
}
static int ledkey_release (struct inode *inode, struct file *filp)
{
    printk( "call release \n" );
    return 0;
}

struct file_operations ledkey_fops =
{
    .owner    = THIS_MODULE,
    .read     = ledkey_read,     
    .write    = ledkey_write,    
	.unlocked_ioctl = ledkey_ioctl,
	.poll	  = ledkey_poll,
    .open     = ledkey_open,     
    .release  = ledkey_release,  
};
static ssize_t proc_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char kbuf[10];
	int ret;
	int len=0; 
	kbuf[0] = sw_no+0x30;
	kbuf[1] = '\n';
	kbuf[2] = 0;
	len = strlen(kbuf);
	ret=copy_to_user(buf,kbuf,len);
	sw_no = 0;
 	*f_pos = -1;
    return len;
}

static ssize_t proc_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	char kbuf[10];
	int ret;
//	get_user(kbuf,buf);
//	printk("count %d %s\n",count,buf);
	ret=copy_from_user(kbuf,buf,count);
	led_write(simple_strtoul(kbuf,NULL,10));
    return count;
//	return -EFAULT;
}
struct file_operations proc_led_fops =
{
    .owner    = THIS_MODULE,
    .write    = proc_write,    
};
struct file_operations proc_key_fops =
{
    .owner    = THIS_MODULE,
    .read     = proc_read,     
};

static void mkproc(void)
{
	keyledproc_root_fp  = proc_mkdir( "ledkey", 0 );

	keyledproc_led_fp   = proc_create_data( "led", S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO, keyledproc_root_fp, &proc_led_fops, NULL);
	keyledproc_key_fp   = proc_create_data( "key", S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO, keyledproc_root_fp, &proc_key_fops, NULL);
}
static void rmproc(void)
{
	remove_proc_entry( "led"  , keyledproc_root_fp );
	remove_proc_entry( "key"  , keyledproc_root_fp );
	remove_proc_entry( "ledkey" , 0 );
}
static int ledkey_init(void)
{
    int result;

    printk( "call ledkey_init \n" );    

    result = register_chrdev( LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME, &ledkey_fops);
    if (result < 0) return result;

	led_init();
//	key_init();
	result = key_irq_init();
    if (result < 0) return result;
	mkproc();
    return 0;
}

static void ledkey_exit(void)
{
    printk( "call ledkey_exit \n" );    
    unregister_chrdev( LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME );
	led_exit();
//	key_exit();
	key_irq_exit();
	rmproc();
}

module_init(ledkey_init);
module_exit(ledkey_exit);

MODULE_LICENSE("Dual BSD/GPL");
