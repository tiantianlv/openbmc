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
#define SYSCPLD_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define SYSCPLD_DEBUG(fmt, ...)
#endif


enum chips {
	SYSCPLD = 1,
};


static const struct i2c_device_id syscpld_id[] = {
	{"syscpld", SYSCPLD },
	{ }
};

static i2c_dev_data_st syscpld_data;
static const i2c_dev_attr_st syscpld_attr_table[] = {
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
	  0x1, 0, 8,
	},
	{
	  "sb_reset",
	  "switch board reset control:\n"
	  "0x0: reset\n"
	  "0x1 not reset",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x4, 0, 1,
	},
	{
	  "si_reset",
	  "switch I2C reset control:\n"
	  "0x0: reset\n"
	  "0x1 not reset",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x4, 1, 1,
	},
	{
	  "fan_i2c_reset",
	  "FAN I2C reset control:\n"
	  "0x0: reset\n"
	  "0x1 not reset",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x4, 4, 1,
	},
	{
	  "fan_reset",
	  "FAN reset control:\n"
	  "0x0: reset\n"
	  "0x1 not reset",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x4, 5, 1,
	},
	{
	  "bmc_reset",
	  "BMC reset control:\n"
	  "0x0: reset\n"
	  "0x1 not reset",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x4, 6, 1,
	},
	{
	  "psu_i2c_en1",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0xa, 0, 1,
	},
	{
	  "psu_i2c_en2",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0xa, 1, 1,
	},
	{
	  "psu_i2c_ready1",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0xa, 4, 1,
	},
	{
	  "psu_i2c_ready2",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0xa, 5, 1,
	},
	{
	  "sol_control",
	  "0x0: switch to BMC\n"
	  "0x1: switch to COMe",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0xc, 0, 1,
	},
	{
	  "int_status",
	  "0x0: interrupt\n"
	  "0x1: no interrupt",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x10, 0, 1,
	},
	{
	  "lm75_cb_int1_n",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x11, 0, 1,
	},
	{
	  "fan_int",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x11, 1, 1,
	},
	{
	  "psu1_int",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x11, 2, 1,
	},
	{
	  "psu2_int",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x11, 3, 1,
	},
	{
	  "sw_lm75_1_int",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x11, 4, 1,
	},
	{
	  "sw_lm75_2_int",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x11, 5, 1,
	},
	{
	  "pwr_come_en",
	  "0x0: COMe power is off\n"
	  "0x1: COMe power is on",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x20, 0, 1,
	},
	{
	  "come_rst_n",
	  "0x0: trigger COMe reset\n"
	  "0x1: normal",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x21, 0, 1,
	},
	{
	  "come_status",
	  "0x1: SUS_S3_N\n"
	  "0x2: SUS_S4_N\n"
	  "0x4: SUS_S5_N\n"
	  "0x8: SUS_STAT_N",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x22, 0, 4,
	},
	{
	  "cb_pwr_btn_n",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x24, 0, 1,
	},
	{
	  "cb_type_n",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x25, 0, 3,
	},
	{
	  "switch_power_f",
	  "0x1: force to switch card power off\n"
	  "0x2: force to switch card power on",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x26, 0, 2,
	},
	{
	  "cb_rst_n",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x27, 0, 1,
	},
	{
	  "bios_spi_wp0_n",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x31, 0, 1,
	},
	{
	  "bios_spi_wp1_n",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x31, 1, 1,
	},
	{
	  "tlv_eeprom_wp",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x31, 2, 1,
	},
	{
	  "sys_eeprom_wp",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x31, 3, 1,
	},
	{
	  "fan_ctrl_en",
	  "0x0: force fan to full speed\n"
	  "0x1: disable",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x59, 0, 1,
	},
	{
	  "led_ctrl_en",
	  "0x0: force all fans led to amber\n"
	  "0x1: disable",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x59, 1, 1,
	},
	{
	  "psu_r_en",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x5f, 0, 1,
	},
	{
	  "psu_l_en",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x5f, 1, 1,
	},
	{
	  "psu_r_kill_en",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x5f, 4, 1,
	},
	{
	  "psu_l_kill_en",
	  NULL,
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x5f, 5, 1,
	},
	{
	  "psu_r_status",
	  "0x0: not OK\n"
	  "0x1: OK",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x60, 0, 1,
	},
	{
	  "psu_l_status",
	  "0x0: not OK\n"
	  "0x1: OK",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x60, 1, 1,
	},
	{
	  "psu_r_present",
	  "0x0: present\n"
	  "0x1: absent",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x60, 2, 1,
	},
	{
	  "psu_l_present",
	  "0x0: present\n"
	  "0x1: absent",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x60, 3, 1,
	},
	{
	  "psu_r_laert",
	  "0x0: alert\n"
	  "0x1: not alert",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x60, 4, 1,
	},
	{
	  "psu_l_laert",
	  "0x0: alert\n"
	  "0x1: not alert",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x60, 5, 1,
	},
	{
	  "psu_r_ac_status",
	  "0x0: not OK\n"
	  "0x1: OK",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x60, 6, 1,
	},
	{
	  "psu_l_ac_status",
	  "0x0: not OK\n"
	  "0x1: OK",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x60, 7, 1,
	},
	{
	  "psu_l_led_ctrl_en",
	  "0x0: disable\n"
	  "0x1: enable",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x61, 0, 1,
	},
	{
	  "psu_r_led_ctrl_en",
	  "0x0: disable\n"
	  "0x1: enable",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x61, 1, 1,
	},
	{
	  "sysled_ctrl",
	  "0x0: on\n"
	  "0x1: 1HZ blink\n"
	  "0x2: 4HZ blink\n"
	  "0x3: off",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x62, 0, 2,
	},
	{
	  "sysled_select",
	  "0x0:  green and yellow alternate blink\n"
	  "0x1: green\n"
	  "0x2: yellow\n"
	  "0x3: off",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x62, 4, 2,
	},
	{
	  "led_alarm_ctrl",
	  "0x1: 1HZ blink\n"
	  "0x2: 4HZ blink\n"
	  "other: not blink",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x63, 0, 2,
	},
	{
	  "led_alarm_select",
	  "0x1: green\n"
	  "0x2: yellow\n"
	  "other: off",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x63, 4, 2,
	},
	{
	  "pwr_cycle",
	  "0x0: enable\n"
	  "0x1: disable",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x64, 0, 1,
	},
	{
	  "bios_boot_ok",
	  "0x0: not ok\n"
	  "0x1: ok",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x70, 0, 1,
	},
	{
	  "bios_boot_cs",
	  "0x0: from BIOS0\n"
	  "0x1: from BIOS1",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x70, 1, 1,
	},
	{
	  "boot_counter",
	  NULL,
	  NULL,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x71, 0, 8,
	},
	{
	  "thermal_shutdown_en",
	  "0x0: disable\n"
	  "0x1: enable",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  I2C_DEV_ATTR_STORE_DEFAULT,
	  0x75, 0, 1,
	},
	{
	  "lm75_1_status",
	  "0x0: over temp\n"
	  "0x1: normal",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x76, 0, 1,
	},
	{
	  "lm75_2_status",
	  "0x0: over temp\n"
	  "0x1: normal",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x76, 1, 1,
	},
	{
	  "lm75_bb_status",
	  "0x0: over temp\n"
	  "0x1: normal",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x76, 2, 1,
	},
	{
	  "shutdown_status",
	  "0x0: normal\n"
	  "0x1: shutdown",
	  I2C_DEV_ATTR_SHOW_DEFAULT,
	  NULL,
	  0x76, 4, 1,
	},

};

static int syscpld_remove(struct i2c_client *client)
{
	i2c_dev_sysfs_data_clean(client, &syscpld_data);
	return 0;
}

static int syscpld_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int n_attrs;

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_READ_BYTE_DATA))
		return -ENODEV;

	n_attrs = sizeof(syscpld_attr_table) / sizeof(syscpld_attr_table[0]);

	return i2c_dev_sysfs_data_init(client, &syscpld_data, syscpld_attr_table, n_attrs);
}


static struct i2c_driver syscpld_driver = {
	.driver = {
		   .name = "syscpld",
		   },
	.probe = syscpld_probe,
	.remove = syscpld_remove,
	.id_table = syscpld_id,
};

module_i2c_driver(syscpld_driver);

MODULE_AUTHOR("Micky Zhan@Celestica.com");
MODULE_DESCRIPTION("system CPLD driver for CPLD");
MODULE_LICENSE("GPL");

