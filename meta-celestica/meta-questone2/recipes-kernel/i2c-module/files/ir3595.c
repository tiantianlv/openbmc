/*
 * IR358x driver for power and compatibles
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
#include "pmbus.h"
#include "i2c_dev_sysfs.h"

#define DEBUG

#ifdef DEBUG
#define IR3595_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define IR3595_DEBUG(fmt, ...)
#endif


enum chips {
	IR3595 = 1,
};


static const struct i2c_device_id ir3595_id[] = {
	{"ir3595", IR3595 },

	{ }
};

static ssize_t ir3595_vout_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
	int value = -1;
	int count = 10;
	
	while((value < 0 || value == 0xffff) && count--) {
		value = i2c_dev_read_word_bigendian(dev, attr);
	}
	
	if (value < 0) {
		IR3595_DEBUG("I2C read error, return %d!\n", value);
		return -1;
	}
	
	value = (value * 1000) / 2048;
	
	return scnprintf(buf, PAGE_SIZE, "%d\n", value);
}

static ssize_t ir3595_iout_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
	int value = -1;
	int count = 10;
	
	while((value < 0 || value == 0xff) && count--) {
		value = i2c_dev_read_byte(dev, attr);
	}
	
	if (value < 0) {
		IR3595_DEBUG("I2C read error, return %d!\n", value);
		return -1;
	}
	
	value = value * 2 * 1000;
	
	return scnprintf(buf, PAGE_SIZE, "%d\n", value);
}




static i2c_dev_data_st ir3595_data;
static const i2c_dev_attr_st ir3595_attr_table[] = {
	{
	  "in0_input",
	  NULL,
	  ir3595_vout_show,
	  NULL,
	  0x9a, 0, 8,
	},
	{
	  "curr1_input",
	  NULL,
	  ir3595_iout_show,
	  NULL,
	  0x94, 0, 8,
	},
	{
	  "in0_label",
	  "Switch chip Voltage",
	  i2c_dev_show_label,
	  NULL,
	  0x0, 0, 0,
	},
	{
	  "curr1_label",
	  "Switch chip Current",
	  i2c_dev_show_label,
	  NULL,
	  0x0, 0, 0,
	},
};

static int ir3595_remove(struct i2c_client *client)
{
	i2c_dev_sysfs_data_clean(client, &ir3595_data);
	return 0;
}

static int ir3595_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int n_attrs;

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_READ_BYTE_DATA
			| I2C_FUNC_SMBUS_READ_WORD_DATA))
		return -ENODEV;

	n_attrs = sizeof(ir3595_attr_table) / sizeof(ir3595_attr_table[0]);

	return i2c_dev_sysfs_data_init(client, &ir3595_data, ir3595_attr_table, n_attrs);
}


static struct i2c_driver ir3595_driver = {
	.driver = {
		   .name = "ir3595",
		   },
	.probe = ir3595_probe,
	.remove = ir3595_remove,
	.id_table = ir3595_id,
};

module_i2c_driver(ir3595_driver);

MODULE_AUTHOR("Micky Zhan@Celestica.com");
MODULE_DESCRIPTION("FRU driver for PSU");
MODULE_LICENSE("GPL");

