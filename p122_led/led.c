#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>

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

void led_read(unsigned long * led_data)
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
	//led_write(data);
	return;
}

static int led_init(void)
{
	int ret;
	printk(KERN_DEBUG "Hello world [ledvalue=%ld]\n", ledvalue);
	ret = led_request();
	if(ret<0)
	{
//		return -EBUSY; /* Device or resource busy */
		return ret; 
	}
	led_write(ledvalue);
	return 0;
}

static void led_exit(void)
{
	printk(KERN_DEBUG "Goodbye, world\n");
	led_write(0);
	led_free();
}


module_init(led_init);
module_exit(led_exit);

MODULE_AUTHOR("Bae Cha-In baecain@gmail.com");
MODULE_DESCRIPTION("Module Parameter Test Led Module");
MODULE_LICENSE("Dual BSD/GPL");
