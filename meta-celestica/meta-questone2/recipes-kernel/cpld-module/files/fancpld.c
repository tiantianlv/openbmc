/*
 * SYS CPLD driver for CPLD and compatibles
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */



#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/i2c/pmbus.h>
#include "i2c_dev_sysfs.h"

#define DEBUG

#ifdef DEBUG
#define FANCPLD_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define FANCPLD_DEBUG(fmt, ...)
#endif


enum chips {
	FANCPLD = 1,
};


static const struct i2c_device_id fancpld_id[] = {
	{"fancpld", FANCPLD },
	{ }
};

static i2c_dev_data_st fancpld_data;
static const i2c_dev_attr_st fancpld_attr_table[] = {
	{
	  "version",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x0, 0, 8,
	},
	{
	  "scratch",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x4, 0, 8,
	},
	{
	  "int_status",
	  "0x0: interrupt\n"
	  "0x1: not interrupt",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x6, 0, 1,
	},
	{
	  "int_result",
	  "0x0: not interrupt\n"
	  "0x1: interrupt",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x7, 0, 1,
	},
	{
	  "eeprom_wp",
	  "0x0: protect\n"
	  "0x1: not protect",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x9, 0, 1,
	},
	{
	  "wdt_en",
	  "0x0: fan wdt disable\n"
	  "0x1: fan wdt enable",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0xc, 0, 1,
	},
	{
	  "fan1_input",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x20, 0, 8,
	},
	{
	  "fan2_input",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x21, 0, 8,
	},
	{
	  "fan1_pwm",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x22, 0, 8,
	},
	{
	  "fan1_led",
	  "0x1: green\n"
	  "0x2: red\n"
	  "0x3: off",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x24, 0, 2,
	},
	{
	  "fan1_eeprom_wp",
	  "0x0: protect\n"
	  "0x1: not protect",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x25, 0, 1,
	},
	{
	  "fan1_present",
	  "0x0: present\n"
	  "0x1: absent",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x26, 0, 1,
	},
	{
	  "fan1_dir",
	  "0x0: F2B\n"
	  "0x1: B2F",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x26, 1, 1,
	},
	{
	  "fan3_input",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x30, 0, 8,
	},
	{
	  "fan4_input",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x31, 0, 8,
	},
	{
	  "fan2_pwm",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x32, 0, 8,
	},
	{
	  "fan2_led",
	  "0x1: green\n"
	  "0x2: red\n"
	  "0x3: off",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x34, 0, 2,
	},
	{
	  "fan2_eeprom_wp",
	  "0x0: protect\n"
	  "0x1: not protect",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x35, 0, 1,
	},
	{
	  "fan2_present",
	  "0x0: present\n"
	  "0x1: absent",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x36, 0, 1,
	},
	{
	  "fan2_dir",
	  "0x0: F2B\n"
	  "0x1: B2F",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x36, 1, 1,
	},
	{
	  "fan5_input",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x40, 0, 8,
	},
	{
	  "fan6_input",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x41, 0, 8,
	},
	{
	  "fan3_pwm",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x42, 0, 8,
	},
	{
	  "fan3_led",
	  "0x1: green\n"
	  "0x2: red\n"
	  "0x3: off",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x44, 0, 2,
	},
	{
	  "fan3_eeprom_wp",
	  "0x0: protect\n"
	  "0x1: not protect",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x45, 0, 1,
	},
	{
	  "fan3_present",
	  "0x0: present\n"
	  "0x1: absent",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x46, 0, 1,
	},
	{
	  "fan3_dir",
	  "0x0: F2B\n"
	  "0x1: B2F",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x46, 1, 1,
	},
	{
	  "fan7_input",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x50, 0, 8,
	},
	{
	  "fan8_input",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x51, 0, 8,
	},
	{
	  "fan4_pwm",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x52, 0, 8,
	},
	{
	  "fan4_led",
	  "0x1: green\n"
	  "0x2: red\n"
	  "0x3: off",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x54, 0, 2,
	},
	{
	  "fan4_eeprom_wp",
	  "0x0: protect\n"
	  "0x1: not protect",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x55, 0, 1,
	},
	{
	  "fan4_present",
	  "0x0: present\n"
	  "0x1: absent",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x56, 0, 1,
	},
	{
	  "fan4_dir",
	  "0x0: F2B\n"
	  "0x1: B2F",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x56, 1, 1,
	},
	

};

static int fancpld_remove(struct i2c_client *client)
{
	i2c_dev_sysfs_data_clean(client, &fancpld_data);
	return 0;
}

static int fancpld_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int n_attrs;

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_READ_BYTE_DATA))
		return -ENODEV;

	n_attrs = sizeof(fancpld_attr_table) / sizeof(fancpld_attr_table[0]);

	return i2c_dev_sysfs_data_init(client, &fancpld_data, fancpld_attr_table, n_attrs);
}


static struct i2c_driver fancpld_driver = {
	.driver = {
		   .name = "fancpld",
		   },
	.probe = fancpld_probe,
	.remove = fancpld_remove,
	.id_table = fancpld_id,
};

module_i2c_driver(fancpld_driver);

MODULE_AUTHOR("Micky Zhan@Celestica.com");
MODULE_DESCRIPTION("system CPLD driver for CPLD");
MODULE_LICENSE("GPL");

