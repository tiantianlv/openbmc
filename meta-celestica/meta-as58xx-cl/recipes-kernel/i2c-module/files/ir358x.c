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
#define IR358X_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define IR358X_DEBUG(fmt, ...)
#endif


#define PSU_FRU_WAIT_TIME		1000	/* uS	*/
#define PSU_FRU_DATA_SIZE_MAX  32

#define TO_IR358X_DATA(x)  container_of(x, struct ir358x_data_t, dev_data)
#define SYSFS_READ 0
#define SYSFS_WRITE 1


enum chips {
	IR3581 = 1,
	IR3584,
	IR3595,
	IR38060,
	IR38062,
};

enum alarm {
	IN0_MIN = 1,
	IN0_MAX,
	CURR1_MIN,
	CURR1_MAX,
};


struct alarm_data_t {
	int alarm_min;
	int alarm_max;
};

struct ir358x_alarm_data {
	struct alarm_data_t in0; //Vout
	struct alarm_data_t curr1;
};


struct ir358x_data_t {
	int id;
	i2c_dev_data_st dev_data;
	struct ir358x_alarm_data alarm_data;
};


static const struct i2c_device_id ir358x_id[] = {
	{"ir3581", IR3581 },
	{"ir3584", IR3584 },
	{"ir38060", IR38060 },
	{"ir38062", IR38062 },
	{ }
};


static ssize_t ir358x_vout_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	i2c_dev_data_st *data = i2c_get_clientdata(client);
	i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
	const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
	struct ir358x_data_t *ir358x_data = TO_IR358X_DATA(data);
	int value = -1;
	int result;
	int count = 10;

	mutex_lock(&data->idd_lock);
	while((value < 0 || value == 0xffff) && count--) {
	  value = i2c_smbus_read_word_data(client, (dev_attr->ida_reg));
	}
	mutex_unlock(&data->idd_lock);
	
	if (value < 0) {
	  /* error case */
	  IR358X_DEBUG("I2C read error!\n");
	  return -1;
	}
	if(ir358x_data->id == IR38060 || ir358x_data->id == IR38062
		|| ir358x_data->id == IR3584)
		result = (value * 1000) / 256;
	else
		result = (value * 1000) / 512;
	
	return scnprintf(buf, PAGE_SIZE, "%d\n", result);

}

static ssize_t ir358x_iout_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	i2c_dev_data_st *data = i2c_get_clientdata(client);
	i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
	const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
	int value = -1;
	int result;
	int count = 10;
	
	mutex_lock(&data->idd_lock);
	while((value < 0 || value == 0xffff) && count--) {
	  value = i2c_smbus_read_word_data(client, (dev_attr->ida_reg));
	}
	mutex_unlock(&data->idd_lock);
	
	if (value < 0) {
	  /* error case */
	  IR358X_DEBUG("I2C read error!\n");
	  return -1;
	}
	
	result = ((value & 0x7ff) * 1000)/ 4;
	
	return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}


static int alarm_value_rw(struct ir358x_data_t *data, int reg, int opcode, int value)
{
	int *p = NULL;

	switch(reg) {
		case IN0_MIN:
			p = &data->alarm_data.in0.alarm_min;
			break;
		case IN0_MAX:
			p = &data->alarm_data.in0.alarm_max;
			break;
		case CURR1_MIN:
			p = &data->alarm_data.curr1.alarm_min;
			break;
		case CURR1_MAX:
			p = &data->alarm_data.curr1.alarm_max;
			break;
		default:
			return -1;
	}

	if(opcode == SYSFS_READ)
		return *p;
	else if(opcode == SYSFS_WRITE)
		*p = value;
	else
		return -1;

	return 0;
}
static ssize_t ir358x_alarm_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	int value = -1;
	struct i2c_client *client = to_i2c_client(dev);
	i2c_dev_data_st *data = i2c_get_clientdata(client);
	i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
	const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
	struct ir358x_data_t *ir358x_data = TO_IR358X_DATA(data);
	struct ir358x_alarm_data alarm_data;

	if(!ir358x_data)
		return -1;

	value = alarm_value_rw(ir358x_data, dev_attr->ida_reg, SYSFS_READ, 0);

	return scnprintf(buf, PAGE_SIZE, "%d\n", value);
}

static int ir358x_alarm_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	int write_value = 0;
	struct i2c_client *client = to_i2c_client(dev);
	i2c_dev_data_st *data = i2c_get_clientdata(client);
	i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
	const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
	struct ir358x_data_t *ir358x_data = TO_IR358X_DATA(data);
	struct ir358x_alarm_data alarm_data;

	if(!ir358x_data)
		return -1;

	if (buf == NULL) {
		return -ENXIO;
	}

	rc = kstrtol(buf, 10, &write_value);
	if (rc != 0)	{
		return count;
	}
	rc = alarm_value_rw(ir358x_data, dev_attr->ida_reg, SYSFS_WRITE, write_value);
	if(rc < 0)
		return -1;

	return count;
}





static const i2c_dev_attr_st ir358x_attr_table[] = {
	{
	  "in0_input",
	  NULL,
	  ir358x_vout_show,
	  NULL,
	  0x8b, 0, 8,
	},
	{
	  "curr1_input",
	  NULL,
	  ir358x_iout_show,
	  NULL,
	  0x8c, 0, 8,
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
	{
	  "in0_min",
	  NULL,
	  ir358x_alarm_show,
	  ir358x_alarm_store,
	  IN0_MIN, 0, 0,
	},
	{
	  "in0_max",
	  NULL,
	  ir358x_alarm_show,
	  ir358x_alarm_store,
	  IN0_MAX, 0, 0,
	},
	{
	  "curr1_min",
	  NULL,
	  ir358x_alarm_show,
	  ir358x_alarm_store,
	  CURR1_MIN, 0, 0,
	},
	{
	  "curr1_max",
	  NULL,
	  ir358x_alarm_show,
	  ir358x_alarm_store,
	  CURR1_MAX, 0, 0,
	},

};

static int ir358x_remove(struct i2c_client *client)
{
	i2c_dev_data_st *data = i2c_get_clientdata(client);
	struct ir358x_data_t *ir358x_data = TO_IR358X_DATA(data);

	i2c_dev_sysfs_data_clean(client, &ir358x_data->dev_data);
	kfree(ir358x_data);
	return 0;
}

static int ir358x_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int n_attrs;
	struct ir358x_data_t *data;

	client->flags |= I2C_CLIENT_PEC;
	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_READ_BYTE_DATA
			| I2C_FUNC_SMBUS_READ_WORD_DATA))
		return -ENODEV;

	n_attrs = sizeof(ir358x_attr_table) / sizeof(ir358x_attr_table[0]);

	data = devm_kzalloc(&client->dev, sizeof(struct ir358x_data_t), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->id = id->driver_data;

	return i2c_dev_sysfs_data_init(client, &data->dev_data, ir358x_attr_table, n_attrs);
}


static struct i2c_driver ir358x_driver = {
	.driver = {
		   .name = "ir358x",
		   },
	.probe = ir358x_probe,
	.remove = ir358x_remove,
	.id_table = ir358x_id,
};

module_i2c_driver(ir358x_driver);

MODULE_AUTHOR("Micky Zhan@Celestica.com");
MODULE_DESCRIPTION("FRU driver for PSU");
MODULE_LICENSE("GPL");

