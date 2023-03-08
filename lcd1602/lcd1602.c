#include <linux/module.h>
#include <linux/i2c.h>
//#include <delay.h>

#define CLEAR_DISPLAY 1
#define LCD_WIDTH 20
#define LCD_ROWS 2

//#########
#define DEBUG

// maximum length of string in lcd
#define LCD_STR_MAX_LEN (50)

#define ENABLE_BIT 		(0x01 << 2) // Enable bit
#define REGSELECT_BIT 	(0x01)  // Register select bit

// flags for backlight control
#define LCD_BACKLIGHT  0x08
#define LCD_NOBACKLIGHT  0x00

// other commands
#define LCD_CLEARDISPLAY  0x01
#define LCD_RETURNHOME  0x02
#define LCD_ENTRYMODESET  0x04
#define LCD_DISPLAYCONTROL  0x08
#define LCD_CURSORSHIFT  0x10
#define LCD_FUNCTIONSET  0x20
#define LCD_SETCGRAMADDR  0x40
#define LCD_SETDDRAMADDR  0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT  0x00
#define LCD_ENTRYLEFT  0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for function set
#define LCD_8BITMODE  0x10
#define LCD_4BITMODE  0x00
#define LCD_2LINE  0x08
#define LCD_1LINE  0x00
#define LCD_5x10DOTS  0x04
#define LCD_5x8DOTS  0x00

struct lcd1602
{
	struct i2c_client *client;
	struct device* dev;
	char curr_val[LCD_STR_MAX_LEN];
	ssize_t curr_val_len;
};


static int lcd_write_4bits(struct i2c_client *client, char data)
{
	/*#### why here a buffer with one element ### ? */
	char buffer[1];
	int ret;

	buffer[0]= data | ENABLE_BIT | LCD_BACKLIGHT;
	ret = i2c_master_send(client, buffer, sizeof(buffer));
	if (ret<0)
		return ret;
	//msleep(5);
	
	buffer[0]= (data & ~ENABLE_BIT) | LCD_BACKLIGHT;
	ret = i2c_master_send(client, buffer, sizeof(buffer));
	//msleep(5);
	
	return ret;
}

static int lcd_write_cmd(struct device* dev, char cmd, char mode)
{
	int ret;
	struct lcd1602* data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	
	ret = lcd_write_4bits(client, mode | (cmd & 0xF0));
	if (ret<0)
		return ret;

	ret = lcd_write_4bits(client, mode | ((cmd << 4)& 0xF0));
	
	return ret;
}

int lcd_clear(struct device* dev)
{
	return lcd_write_cmd(dev, CLEAR_DISPLAY, REGSELECT_BIT);
}

ssize_t lcd_write_string(struct device* dev, const char* string, const ssize_t str_len, const int line)
{
	ssize_t ret;
	ssize_t cnt;

	uint8_t lines [4] = {
		0x80,
		0xC0,
		0x94,
		0xD4
	};

	lcd_clear(dev);

	printk(KERN_INFO ">>>> string = %s, %x\n", string, str_len );

/* ### Do we need this !! */
	ret = lcd_write_cmd(dev, lines [line-1],0x00);
	if (ret < 0)
		return ret;

	for (cnt=0; cnt< str_len; cnt++)
	{
		
		printk(KERN_INFO ">>>> character = %c, %x\n", string[cnt], string[cnt]);
		/*### here see the ord written in the python code #####*/
		ret = lcd_write_cmd(dev, string[cnt],REGSELECT_BIT);
		if (ret < 0)
			return ret;
	}

	return cnt;
}


#if 0
ssize_t lcd_text(struct device* dev, const char* string, const ssize_t str_len, const int line)
{
	// devide the text to 2 part
	char* line1 [LCD_WIDTH ];
	char* line2 [LCD_WIDTH ];

	ssize_t len_to_write= str_len<LCD_WIDTH ? str_len:LCD_WIDTH;
	
	
	ssize_t remain_to_write = str_len - len_to_write;

	strcpy(line1, string, len_to_write);
	lcd_write_string(dev, line1,len_to_write, line);

	lcd_text(dev, line2,remain_to_write,line++);

}
#endif


int lcd_init(struct device *dev)
{
	lcd_write_cmd(dev, 0x33,0);
    lcd_write_cmd(dev, 0x32,0);
    lcd_write_cmd(dev, 0x06,0);
    lcd_write_cmd(dev, 0x0C,0);
    lcd_write_cmd(dev, 0x28,0);
    lcd_write_cmd(dev, CLEAR_DISPLAY,0);

	return 0;
}

static ssize_t show_lcd_content(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct lcd1602* data = dev_get_drvdata(dev);
	memcpy(buf, data->curr_val, data->curr_val_len);
	return data->curr_val_len;
}

static ssize_t store_lcd_content(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t len)
{
	struct lcd1602* data = dev_get_drvdata(dev);
	data->curr_val_len = (len > LCD_STR_MAX_LEN)?LCD_STR_MAX_LEN:len;
	// write the string in the internal struct
	//###char* test = "abcdefghijklmno";
	//###dev_dbg(dev, ">>> string : %s", buf);
	strcpy(data->curr_val, buf);
	printk(KERN_INFO"data->curr_val = %s, strlen(data->curr_val) =%i len = %i\n",data->curr_val,strlen(data->curr_val), len);
	//return lcd_write_string(dev, data->curr_val, data->curr_val_len, 1);
	return lcd_write_string(dev, data->curr_val, strlen(data->curr_val)-1, 1) + 1;
}

static DEVICE_ATTR(lcd_content, S_IRUGO | S_IWUSR,
		show_lcd_content, store_lcd_content);

int lcd1602_probe(struct i2c_client *client, const struct i2c_device_id* id)
{
	struct lcd1602* lcd_data;
	char* init_str= "lcd init";
	int ret=0;

	printk(KERN_INFO "probe lcd, client->addr = %0x \n", client->addr);

	lcd_data= devm_kzalloc(&client->dev,sizeof(struct lcd1602),GFP_KERNEL);

	if (!lcd_data)
		return -ENOMEM;

	printk(KERN_INFO ">>> 1\n");
	dev_set_drvdata(&client->dev, lcd_data);

	printk(KERN_INFO ">>> 2\n");
	lcd_data->client = client;
	lcd_data->dev = &client->dev;

	lcd_init(&client->dev);

	printk(KERN_INFO ">>> 3\n");

	ret = device_create_file(&client->dev, &dev_attr_lcd_content);
	if (ret) {
		dev_err(&client->dev, "failed: create sysfs entry\n");
		goto err;
	}
	printk(KERN_INFO ">>> 4\n");

	lcd_write_string(&client->dev, init_str, strlen(init_str) ,1);

	return 0;
err:
	device_remove_file(&client->dev, &dev_attr_lcd_content);
	return ret;
}

int lcd1602_remove(struct i2c_client *client)
{
	device_remove_file(&client->dev, &dev_attr_lcd_content);
	return 0;
}

static const struct of_device_id lcd1602_of_match[] = {
	{.compatible = "lcd1602",},
	{/* sentinel*/ },
};

MODULE_DEVICE_TABLE(of,lcd1602_of_match);

static struct i2c_driver lcd1602_driver = {
	.driver = {
		.name = "lcd1602",
		.of_match_table = lcd1602_of_match,
	},
	.probe = lcd1602_probe,
	.remove = lcd1602_remove,
};

module_i2c_driver(lcd1602_driver);

MODULE_DESCRIPTION("lcd1602 driver");
MODULE_AUTHOR("Achraf Boussetta <boussettaachraf26@gmail.com>");
MODULE_LICENSE("GPL");


