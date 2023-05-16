#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       

#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>

#define   MINOR_DEV_NAME        "minordev"
#define   MINOR_DEV_MAJOR            240
#define	  DEBUG 0
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))

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

static int led[] = {
    IMX_GPIO_NR(1, 16),
    IMX_GPIO_NR(1, 17),
    IMX_GPIO_NR(1, 18),
    IMX_GPIO_NR(1, 19),
};

static void led_cont(unsigned long data)
{
    int i;
    for(i = 0; i < ARRAY_SIZE(led); i++){
        gpio_set_value(led[i], ((data >> i) & 0x01));
#if DEBUG
        printk("#### %s, data = %ld(%ld)\n", __FUNCTION__, data,(data>>i) & 0x01);
#endif
    }
}
static unsigned char key_cont(void)
{
    int i;
    int ret;
    unsigned char sw=0;
    for(i = ARRAY_SIZE(key)-1; i >= 0; i--){
        ret = gpio_get_value(key[i]);
        sw |=ret;
        if(i==0)
            break;
        sw <<= 1;  //sw = sw << 1;
#if DEBUG
        printk("#### %s, %d = %d(%u)\n", __FUNCTION__,i, ret,(sw));
#endif
    }
	return sw;
}
static int minor1_open (struct inode *inode, struct file *filp)
{
    printk( "call minor1_open\n" );

    return 0;
}

ssize_t minor1_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int ret;
	unsigned char led;
 	printk( "call minor1_write\n" );
	ret=copy_from_user(&led,buf,count);
	led_cont(led);
    return count;
}

static int minor1_release (struct inode *inode, struct file *filp)
{
    printk( "call minor1_release\n" );    

    return 0;
}

static int minor2_open (struct inode *inode, struct file *filp)
{
    printk( "call minor2_open\n" );

    return 0;
}

static ssize_t minor2_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	int ret;
	unsigned char sw;
//  	printk( "call minor2_read\n" );
	sw = key_cont();
	ret=copy_to_user(buf,&sw,count);
    return count;
}

static int minor2_release (struct inode *inode, struct file *filp)
{
    printk( "call minor2_release\n" );

    return 0;
}

struct file_operations minor1_fops =
{
    .owner    = THIS_MODULE,
    .write    = minor1_write,
    .open     = minor1_open,
    .release  = minor1_release,
};

struct file_operations minor2_fops =
{
    .owner    = THIS_MODULE,
    .read     = minor2_read,
    .open     = minor2_open,
    .release  = minor2_release,
};

static int minor_open (struct inode *inode, struct file *filp)
{
    printk( "call minor_open\n" );
    switch (MINOR(inode->i_rdev)) 
    {
    case 1: filp->f_op = &minor1_fops; break;
    case 2: filp->f_op = &minor2_fops; break;
    default : return -ENXIO;
    }
    
    if (filp->f_op && filp->f_op->open)
        return filp->f_op->open(inode,filp);
        
    return 0;
}

static int key_init(void)
{
	int i;
	int ret = 0;
	for (i = 0; i < ARRAY_SIZE(key); i++) {
		ret = gpio_request(key[i], "gpio key");
		if(ret){
			printk("#### FAILED Request gpio %d. error : %d \n", key[i], ret);
		} 
		else {
			gpio_direction_input(key[i]);
		}
	}
	return ret;
}
static int led_init(void)
{
	int ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(led); i++) {
		ret = gpio_request(led[i], "gpio led");
		if(ret){
			printk("#### FAILED Request gpio %d. error : %d \n", led[i], ret);
		} 
		else {
			gpio_direction_output(led[i], 0);
		}
	}
	return ret;
}
static void key_free(void)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(key); i++){
        gpio_free(key[i]);
    }
}
static void led_free(void)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(led); i++){
        gpio_free(led[i]);
    }
}
struct file_operations minor_fops =
{
    .owner    = THIS_MODULE,
    .open     = minor_open,     
};

static int minor_init(void)
{
    int result;
	
	key_init();
	led_init();
    result = register_chrdev( MINOR_DEV_MAJOR, MINOR_DEV_NAME, &minor_fops);
    if (result < 0) return result;

    return 0;
}

static void minor_exit(void)
{
	key_free();
   	led_free();
    unregister_chrdev( MINOR_DEV_MAJOR, MINOR_DEV_NAME );
}

module_init(minor_init);
module_exit(minor_exit);

MODULE_LICENSE("Dual BSD/GPL");
