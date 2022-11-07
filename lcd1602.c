#include <linux/module.h>
#include <linux/i2c.h>

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
};

static int lcd_write_4bits(struct i2c_client *client, char data)
{
	char buffer[1];
	int ret;

	buffer[0]= data | ENABLE_BIT | LCD_BACKLIGHT;
	ret = i2c_master_send(client, buffer, sizeof(buffer));
	if (ret<0)
		return ret;
	
	buffer[0]= (data & ~ENABLE_BIT) | LCD_BACKLIGHT;
	ret = i2c_master_send(client, buffer, sizeof(buffer));
	
	return ret;
}

static int lcd_write_cmd(struct i2c_client *client, char cmd, char mode)
{
	int ret;
	
	ret = lcd_write_4bits(client, mode | (cmd & 0xF0));
	if (ret<0)
		return ret;

	ret = lcd_write_4bits(client, mode | ((cmd << 4)& 0xF0));
	
	return ret;
}

static int lcd_write_string(struct i2c_client *client, const char* string, const int str_len, const int line)
{
	int ret;

	switch (line)
	{
		case 1:
			lcd_write_cmd(client, 0x80,0x00);
			break;
		case 2:
			lcd_write_cmd(client, 0xC0,0x00);
			break;
		case 3:
			lcd_write_cmd(client, 0x94,0x00);
			break;
		case 4:
			lcd_write_cmd(client, 0xD4,0x00);
			break;
		default:
			printk(KERN_ERR, "incorrect line number %d", line);
			return -1;/*### to be changed, see the errors provided by the kernel ###*/
			break;
	}

	int cnt;
	for (cnt=0; cnt< str_len; cnt++)
	{
		/*### here see the ord written in the python code #####*/
		lcd_write_cmd(client, string[cnt],REGSELECT_BIT);
	}
	
	return ret;
}


int lcd_init(struct i2c_client *client)
{
	lcd_write_cmd(client, 0x03,0x00 );
	lcd_write_cmd(client, 0x03,0x00 );
	lcd_write_cmd(client, 0x03,0x00 );
	lcd_write_cmd(client, 0x02,0x00 );

	lcd_write_cmd(client, LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS | LCD_4BITMODE,0x00);
    lcd_write_cmd(client, LCD_DISPLAYCONTROL | LCD_DISPLAYON,0x00);
    lcd_write_cmd(client, LCD_CLEARDISPLAY,0x00);
    lcd_write_cmd(client, LCD_ENTRYMODESET | LCD_ENTRYLEFT,0x00);

	return 0;
}


int lcd1602_probe(struct i2c_client *client, const struct i2c_device_id* id)
{
	struct lcd1602* lcd;

	printk(KERN_INFO "probe lcd !!! \n");
	printk(KERN_INFO "client->addr = %0x \n", client->addr);

	lcd= devm_kzalloc(&client->dev,sizeof(struct lcd1602),GFP_KERNEL);

	if (!lcd)
		return -ENOMEM;

	lcd->client = client;
	lcd->dev = &client->dev;

	lcd_init(client);

	lcd_write_string(client, "lcd init :)", 5 ,1);

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
};

module_i2c_driver(lcd1602_driver);

MODULE_DESCRIPTION("lcd1602 driver");
MODULE_AUTHOR("Achraf Boussetta <boussettaachraf26@gmail.com>");
MODULE_LICENSE("GPL");