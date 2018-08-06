/*
 * Hardware monitoring driver for DPS1100 and compatibles
 * Based on the pfe3000 driver with the following copyright:
 *
 * Copyright (c) 2011 Ericsson AB.
 * Copyright (c) 2012 Guenter Roeck
 * Copyright 2004-present Facebook. All Rights Reserved.
 * Copyright 2018 Celestica.
 *
 *
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

#define DPS1100_OP_REG_ADDR     PMBUS_OPERATION
#define DPS1100_OP_SHUTDOWN_CMD 0x0
#define DPS1100_OP_POWERON_CMD  0x80


#define DPS1100_WAIT_TIME		1000	/* uS	*/

#define to_dps1100_data(x)  container_of(x, struct dps1100_data, info)



enum chips {
	DPS550 = 1,
	DPS1100,
};

struct dps1100_data {
	int id;
	int shutdown_state;
	struct pmbus_driver_info info;
};


static const struct i2c_device_id dps1100_id[] = {
	{"dps550", DPS550 },
	{"dps1100", DPS1100 },
	{ }
};

static ssize_t dps1100_shutdown_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	const struct pmbus_driver_info *info = pmbus_get_driver_info(client);
	struct dps1100_data *data = to_dps1100_data(info);
	int len = 0;
	u8 read_val = 0;

	read_val = pmbus_read_byte_data(client, 0, DPS1100_OP_REG_ADDR);
	if (read_val >= 0)
	{
		if (read_val == DPS1100_OP_SHUTDOWN_CMD)
			data->shutdown_state = 1;
		else
			data->shutdown_state = 0;

	}

  len = sprintf(buf, "%d\n\nSet to 1 for shutdown DPS1100 PSU\n",
		              data->shutdown_state);
  return len;
}

static int dps1100_shutdown_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	const struct pmbus_driver_info *info = pmbus_get_driver_info(client);
	struct dps1100_data *data = to_dps1100_data(info);

	u8 write_value = 0;
	long shutdown = 0;
	int rc = 0;

	if (buf == NULL) {
		return -ENXIO;
	}

	rc = kstrtol(buf, 10, &shutdown);
	if (rc != 0)	{
		return count;
	}

	if (shutdown == 1) {
		write_value = DPS1100_OP_SHUTDOWN_CMD;
	} else {
		write_value = DPS1100_OP_POWERON_CMD;
	}

	rc = pmbus_write_byte_data(client, 0, DPS1100_OP_REG_ADDR, write_value);
	if (rc == 0) {
		data->shutdown_state = 1;
	}

	return count;
}


static DEVICE_ATTR(shutdown, S_IRUGO | S_IWUSR,
		dps1100_shutdown_show, dps1100_shutdown_store);

static struct attribute *shutdown_attrs[] = {
	 &dev_attr_shutdown.attr,
	 NULL
};
static struct attribute_group control_attr_group = {
     .name = "control",
     .attrs = shutdown_attrs,
};

static int dps1100_register_shutdown(struct i2c_client *client,
			const struct i2c_device_id *id)
{
  return sysfs_create_group(&client->dev.kobj, &control_attr_group);
}

static void dps1100_remove_shutdown(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &control_attr_group);
	return;
}


static int dps1100_remove(struct i2c_client *client)
{
	dps1100_remove_shutdown(client);
	return pmbus_do_remove(client);
}

static int dps1100_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret = 0;
	int kind;
	struct device *dev = &client->dev;
	struct dps1100_data *data;
	struct pmbus_driver_info *info;

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_READ_WORD_DATA | I2C_FUNC_SMBUS_READ_BLOCK_DATA))
		return -ENODEV;

	data = devm_kzalloc(&client->dev, sizeof(struct dps1100_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->shutdown_state = 0;

	info = &data->info;
	info->delay = DPS1100_WAIT_TIME;
	info->pages = 1;
	info->func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_STATUS_INPUT
	  | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
	  | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT
	  | PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_TEMP
	  | PMBUS_HAVE_PIN | PMBUS_HAVE_POUT
	  | PMBUS_HAVE_FAN12 | PMBUS_HAVE_STATUS_FAN12
	  | PMBUS_HAVE_IIN | PMBUS_HAVE_TEMP2;

	info->read_word_data = pmbus_read_word_data;
	info->write_word_data = pmbus_write_word_data;
	info->read_byte_data = pmbus_read_byte_data;
	info->write_byte = pmbus_write_byte;

	ret = dps1100_register_shutdown(client, id);
	if(ret < 0) {
		dev_err(&client->dev, "Unsupported shutdown operation\n");
		return -EIO;
	}


	return pmbus_do_probe(client, id, info);
}


static struct i2c_driver dps1100_driver = {
	.driver = {
		   .name = "dps1100",
		   },
	.probe = dps1100_probe,
	.remove = dps1100_remove,
	.id_table = dps1100_id,
};

module_i2c_driver(dps1100_driver);

MODULE_AUTHOR("Micky Zhan, based on work by Guenter Roeck");
MODULE_DESCRIPTION("PMBus driver for DPS1100");
MODULE_LICENSE("GPL");

