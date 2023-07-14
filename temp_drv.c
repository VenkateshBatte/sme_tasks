#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>



static DEFINE_MUTEX(dht2x_mutex);

unsigned int gpio[]={16,25,17,27,22,5,6,26,23,24,};

void lcd_enable(void){
	gpio_set_value(gpio[0],1);
	msleep(10);
	gpio_set_value(gpio[0],0);
}

void lcd_send_byte(char data) {
	for(int i=0;i<8;i++)
		gpio_set_value(gpio[i+2],((data) & (1<<i)) >> i);
	lcd_enable();
	msleep(5);
}

void lcd_command(uint8_t data){
	gpio_set_value(gpio[1], 0);
	lcd_send_byte(data);
}

void lcd_data(uint8_t data){
	gpio_set_value(gpio[1], 1);
	lcd_send_byte(data);
}

void display_init(void){
	lcd_command(0x30);  
	msleep(10);
	lcd_command(0x0f);
	lcd_command(0x06);
	lcd_command(0x01); 
	msleep(4);
	lcd_command(0x80);
	lcd_command(0x8a);
	lcd_command(0x03);
}

char ack[17]="\0";

static ssize_t dht2x_ack_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{

	if (mutex_lock_interruptible(&dht2x_mutex))
		return -EINTR;

	sscanf(buf,"%s",ack);
	pr_info("ack = %s buf = %s",ack,buf);	
	lcd_command(0x01);	
	lcd_command(0x03);
	for (int i=0;ack[i]!='\0';i++)
		lcd_data(ack[i]);

	mutex_unlock(&dht2x_mutex);
	return count;
}

static ssize_t dht2x_ack_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	int n;

	if (mutex_lock_interruptible(&dht2x_mutex))
		return -EINTR;

	n=sprintf(buf,"%s",ack);

	mutex_unlock(&dht2x_mutex);
	return n;
}

static DEVICE_ATTR_RW(dht2x_ack);	


static ssize_t dht2x_temp_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = container_of(dev,struct i2c_client, dev);
	int n;
	u32 raw_temp,temp;
	u8 result[7];
	u8 write_cmd, read_cmd, cal_cmd, p1, p2;
	
	write_cmd = 0x70;
	cal_cmd   = 0xAC;
	p1	  = 0x33;
	p2	  = 0x00;
	
	
	i2c_master_send(client, &write_cmd,(u8)sizeof(u8));
	i2c_master_send(client, &cal_cmd,(u8)sizeof(u8));
	i2c_master_send(client, &p1,(u8)sizeof(u8));
	i2c_master_send(client, &p2,(u8)sizeof(u8));
	msleep(100);

	read_cmd  = 0x71;
	i2c_master_send(client, &read_cmd,(u8)sizeof(u8));
	i2c_master_recv(client, result, sizeof(result));

    	raw_temp = (((u32)result[3] & 0xf) << 16) + ((u32)result[4] << 8) + (u32)result[5];
	temp = ((200 * raw_temp) / (1024*1024)) - 50;

	n = snprintf(buf, 40, "Temperature in Celsius : %u \n", temp);

    	pr_info("Temparature read successfully.....\n");
	return n;
}
static DEVICE_ATTR_RO(dht2x_temp);	

static int dht2x_probe(struct i2c_client *client,const struct i2c_device_id *id){
	int ret,i;
	struct device dev=client->dev;
	char *names[] = {"ENABLE", "REGISTER_SELECT", "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7"};

	for (i=0;i<10;i++){
		if(gpio_request(gpio[i],names[i])<0){
			pr_info("ERROR: GPIO %d request",gpio[i]);
			goto gpioInit;
		}
		if(gpio_direction_output(gpio[i],0)){
			pr_info("ERROR: In setting GPIO %d direction",gpio[i]);
			goto gpioDir;
		}
	}
	
	display_init();

	ret = sysfs_create_file(&dev.kobj, &dev_attr_dht2x_temp.attr);
   	ret = sysfs_create_file(&dev.kobj, &dev_attr_dht2x_ack.attr);

    	pr_info("DHT2x device detected......!");
	return 0;

gpioDir:
	i=9;
gpioInit:
	for(;i>=0;i--)
		gpio_free(gpio[i]);
	return -1;

};

static void dht2x_remove(struct i2c_client *client)
{
	struct device dev=client->dev;
	
	lcd_command(0x01);
	for (int i=0;i<10;i++){
		gpio_set_value(gpio[i],0);
		gpio_free(gpio[i]);
	}

	sysfs_remove_file(&dev.kobj, &dev_attr_dht2x_ack.attr);
	sysfs_remove_file(&dev.kobj, &dev_attr_dht2x_temp.attr);

	pr_info("DHT2x device removed.....!");	
}

static struct of_device_id dht2x_id[] = {
	{.compatible = "temparature"},
	{ }
};
MODULE_DEVICE_TABLE(of, dht2x_id);

static struct i2c_driver dht2x_driver = {
	.driver     = {
		.name   = "dht2x",
		.of_match_table = dht2x_id,
	},
	.probe		=  	dht2x_probe,
	.remove		=	dht2x_remove,
};

module_i2c_driver(dht2x_driver);


MODULE_AUTHOR("Ken-Sense");
MODULE_DESCRIPTION("Kernel driver for the DHT2x (dht20/21/22) temperature");
MODULE_LICENSE("Dual MIT/GPL");


