/*
 * fand
 *
 * Copyright 2016-present Celestica. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Daemon to manage the fan speed to ensure that we stay within a reasonable
 * temperature range.  We're using a simplistic algorithm to get started:
 *
 * If the fan is already on high, we'll move it to medium if we fall below
 * a top temperature.  If we're on medium, we'll move it to high
 * if the temperature goes over the top value, and to low if the
 * temperature falls to a bottom level.  If the fan is on low,
 * we'll increase the speed if the temperature rises to the top level.
 *
 * To ensure that we're not just turning the fans up, then back down again,
 * we'll require an extra few degrees of temperature drop before we lower
 * the fan speed.
 *
 * We check the RPM of the fans against the requested RPMs to determine
 * whether the fans are failing, in which case we'll turn up all of
 * the other fans and report the problem..
 *
 * TODO:  Implement a PID algorithm to closely track the ideal temperature.
 * TODO:  Determine if the daemon is already started.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <dirent.h>
#include "watchdog.h"
#include <fcntl.h>
#include <openbmc/obmc-i2c.h>

#define CONFIG_PSU_FAN_CONTROL


#define TOTAL_FANS 4
#define TOTAL_PSUS 2
#define FAN_LOW 35
#define FAN_MEDIUM 50
#define FAN_HIGH 100
#define FAN_MAX 100
#define RAISING_TEMP_LOW 26
#define RAISING_TEMP_HIGH 47
#define FALLING_TEMP_LOW 23
#define FALLING_TEMP_HIGH 44
#define SYSTEM_LIMIT 80

#define ALARM_TEMP_THRESHOLD 3
#define ALARM_START_REPORT 3


#define REPORT_TEMP 720  /* Report temp every so many cycles */
#define FAN_FAILURE_OFFSET 30
#define FAN_FAILURE_THRESHOLD 3 /* How many times can a fan fail */
#define FAN_LED_GREEN 1
#define FAN_LED_RED 2
#define SHUTDOWN_DELAY_TIME 72 /*if trigger shutdown event, delay 6mins to shutdown */

#define BAD_TEMP (-60)


#define FAN_DIR_B2F 0
#define FAN_DIR_F2B 1

#define PWM_UNIT_MAX 255

#define PATH_CACHE_SIZE 256

#define FAN_WTD_SYSFS "/sys/bus/i2c/drivers/fancpld/0-000d/fan_wdt_en"
#define PSU1_SHUTDOWN_SYSFS "/sys/bus/i2c/devices/i2c-6/6-0058/control/shutdown"
#define PSU2_SHUTDOWN_SYSFS "/sys/bus/i2c/devices/i2c-6/6-0059/control/shutdown"

#define uchar unsigned char


struct sensor_info_sysfs {
  char* prefix;
  char* suffix;
  uchar temp;
  int (*read_sysfs)(struct sensor_info_sysfs *sensor);
  char path_cache[PATH_CACHE_SIZE];
};

struct fan_info_stu_sysfs {
  char *prefix;
  char *front_fan_prefix;
  char *rear_fan_prefix;
  char *pwm_prefix;
  char *fan_led_prefix;
  char *fan_present_prefix;
  uchar present; //for chassis using, other ignore it
  uchar failed;  //for chassis using, other ignore it
};

struct psu_info_sysfs {
  char* sysfs_path;
  char* shutdown_path;
  int value_to_shutdown;
};


struct board_info_stu_sysfs {
  const char *name;
  uint slot_id;
  int lwarn;
  int hwarn;
  int warn_count;
  struct sensor_info_sysfs *critical;
  struct sensor_info_sysfs *alarm;
};

struct fantray_info_stu_sysfs {
  const char *name;
  int present;
  struct fan_info_stu_sysfs fan1;
};

struct rpm_to_pct_map {
  uint pct;
  uint rpm;
};

static int read_temp_sysfs(struct sensor_info_sysfs *sensor);


static struct sensor_info_sysfs sensor_inlet_u5_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/7-004d",
  .suffix = "temp1_input",
  .temp = 0,
  .read_sysfs = &read_temp_sysfs,
};

static struct sensor_info_sysfs sensor_inlet_u7_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/7-004e",
  .suffix = "temp1_input",
  .temp = 0,
  .read_sysfs = &read_temp_sysfs,
};

static struct sensor_info_sysfs sensor_bcm5870_alarm_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/7-0049",
  .suffix = "temp1_input",
  .temp = 0,
  .read_sysfs = &read_temp_sysfs,
};





/* fantran info*/
static struct fan_info_stu_sysfs fan1_info = {
  .prefix = "/sys/bus/i2c/drivers/fancpld/0-000d",
  .front_fan_prefix = "fan1_input",
  .rear_fan_prefix = "fan2_input",
  .pwm_prefix = "fan1_pwm",
  .fan_led_prefix = "fan1_led",
  .fan_present_prefix = "fan1_present",
  .present = 1,
  .failed = 0,
};

static struct fan_info_stu_sysfs fan2_info = {
  .prefix = "/sys/bus/i2c/drivers/fancpld/0-000d",
  .front_fan_prefix = "fan3_input",
  .rear_fan_prefix = "fan4_input",
  .pwm_prefix = "fan2_pwm",
  .fan_led_prefix = "fan2_led",
  .fan_present_prefix = "fan2_present",
  .present = 1,
  .failed = 0,
};

static struct fan_info_stu_sysfs fan3_info = {
  .prefix = "/sys/bus/i2c/drivers/fancpld/0-000d",
  .front_fan_prefix = "fan5_input",
  .rear_fan_prefix = "fan6_input",
  .pwm_prefix = "fan3_pwm",
  .fan_led_prefix = "fan3_led",
  .fan_present_prefix = "fan3_present",
  .present = 1,
  .failed = 0,
};

static struct fan_info_stu_sysfs fan4_info = {
  .prefix = "/sys/bus/i2c/drivers/fancpld/0-000d",
  .front_fan_prefix = "fan7_input",
  .rear_fan_prefix = "fan8_input",
  .pwm_prefix = "fan4_pwm",
  .fan_led_prefix = "fan4_led",
  .fan_present_prefix = "fan4_present",
  .present = 1,
  .failed = 0,
};

#ifdef CONFIG_PSU_FAN_CONTROL
static struct fan_info_stu_sysfs psu1_fan_info = {
  .prefix = "/sys/bus/i2c/devices/i2c-6/6-0058/hwmon/hwmon5",
  .front_fan_prefix = "fan1_input",
  .rear_fan_prefix = "fan8_input",
  .pwm_prefix = "fan4_pwm",
  .fan_led_prefix = "fan4_led",
  .fan_present_prefix = "fan4_present",
  .present = 1,
  .failed = 0,
};
static struct fan_info_stu_sysfs psu2_fan_info = {
  .prefix = "/sys/bus/i2c/devices/i2c-6/6-0059/hwmon/hwmon5",
  .front_fan_prefix = "fan1_input",
  .rear_fan_prefix = "fan8_input",
  .pwm_prefix = "fan4_pwm",
  .fan_led_prefix = "fan4_led",
  .fan_present_prefix = "fan4_present",
  .present = 1,
  .failed = 0,
};
#endif




/************board and fantray info*****************/
static struct board_info_stu_sysfs board_info[] = {
  {
    .name = "inlet_u5",
    .slot_id = FAN_DIR_B2F,
    .lwarn = 53,
    .hwarn = 58,
    .warn_count = 0,
    .critical = &sensor_inlet_u5_critical_info,
    .alarm = &sensor_inlet_u5_critical_info,
  },
  {
    .name = "inlet_u7",
    .slot_id = FAN_DIR_B2F,
    .lwarn = 53,
    .hwarn = 58,
    .warn_count = 0,
    .critical = &sensor_inlet_u7_critical_info,
    .alarm = &sensor_inlet_u7_critical_info,
  },
  {
    .name = "BCM5870",
    .slot_id = FAN_DIR_B2F,
	.lwarn = 74,
    .hwarn = 77,
    .warn_count = 0,
    .critical = NULL,
    .alarm = &sensor_bcm5870_alarm_info,
  },
  NULL,
};

static struct fantray_info_stu_sysfs fantray_info[] = {
  {
    .name = "Fantry 1",
    .present = 1,
    .fan1 = fan1_info,
  },
  {
    .name = "Fantry 2",
    .present = 1,
    .fan1 = fan2_info,
  },
  {
    .name = "Fantry 3",
    .present = 1,
    .fan1 = fan3_info,
  },
  {
    .name = "Fantry 4",
    .present = 1,
    .fan1 = fan4_info,
  },
#ifdef CONFIG_PSU_FAN_CONTROL
  {
	.name = "PSU 1",
	.present = 1,
	.fan1 = psu1_fan_info,
  },
  {
	.name = "PSU 2",
	.present = 1,
	.fan1 = psu2_fan_info,
  },
#endif
  NULL,
};



#define BOARD_INFO_SIZE (sizeof(board_info) \
                        / sizeof(struct board_info_stu_sysfs))
#define FANTRAY_INFO_SIZE (sizeof(fantray_info) \
                        / sizeof(struct fantray_info_stu_sysfs))

struct rpm_to_pct_map rpm_front_map[] = {{20, 4200},
                                         {25, 5550},
                                         {30, 6180},
                                         {35, 7440},
                                         {40, 8100},
                                         {45, 9300},
                                         {50, 10410},
                                         {55, 10920},
                                         {60, 11910},
                                         {65, 12360},
                                         {70, 13260},
                                         {75, 14010},
                                         {80, 14340},
                                         {85, 15090},
                                         {90, 15420},
                                         {95, 15960},
                                         {100, 16200}};

struct rpm_to_pct_map rpm_rear_map[] = {{20, 2130},
                                        {25, 3180},
                                        {30, 3690},
                                        {35, 4620},
                                        {40, 5130},
                                        {45, 6120},
                                        {50, 7050},
                                        {55, 7560},
                                        {60, 8580},
                                        {65, 9180},
                                        {70, 10230},
                                        {75, 11280},
                                        {80, 11820},
                                        {85, 12870},
                                        {90, 13350},
                                        {95, 14370},
                                        {100, 14850}};

#define FRONT_MAP_SIZE (sizeof(rpm_front_map) / sizeof(struct rpm_to_pct_map))
#define REAR_MAP_SIZE (sizeof(rpm_rear_map) / sizeof(struct rpm_to_pct_map))



/*
 * Initialize path cache by writing 0-length string
 */
static int init_path_cache(void)
{
	int i = 0;
	// Temp Sensor datastructure
	for (i = 0; i < BOARD_INFO_SIZE; i++)
	{
		if (board_info[i].alarm != NULL)
			snprintf(board_info[i].alarm->path_cache, PATH_CACHE_SIZE, "");
		if (board_info[i].critical != NULL)
			snprintf(board_info[i].critical->path_cache, PATH_CACHE_SIZE, "");
	}

	return 0;
}

/*
 * Helper function to probe directory, and make full path
 * Will probe directory structure, then make a full path
 * using "<prefix>/hwmon/hwmonxxx/<suffix>"
 * returns < 0, if hwmon directory does not exist or something goes wrong
 */
int assemble_sysfs_path(const char* prefix, const char* suffix,
                        char* full_name, int buffer_size)
{
	int rc = 0;
	int dirname_found = 0;
	char temp_str[PATH_CACHE_SIZE];
	DIR *dir = NULL;
	struct dirent *ent;

	if (full_name == NULL)
		return -1;

	snprintf(temp_str, (buffer_size - 1), "%s/hwmon", prefix);
	dir = opendir(temp_str);
	if (dir == NULL) {
		rc = ENOENT;
		goto close_dir_out;
	}

	while ((ent = readdir(dir)) != NULL) {
		if (strstr(ent->d_name, "hwmon")) {
			// found the correct 'hwmon??' directory
			snprintf(full_name, buffer_size, "%s/%s/%s",
			       temp_str, ent->d_name, suffix);
			dirname_found = 1;
			break;
		}
	}

close_dir_out:
	if (dir != NULL) {
		closedir(dir);
	}

	if (dirname_found == 0) {
		rc = ENOENT;
	}

	return rc;
}


// Functions for reading from sysfs stub
static int read_sysfs_raw_internal(const char *device, char *value, int log)
{
	FILE *fp;
	int rc, err;

	fp = fopen(device, "r");
	if (!fp) {
		if (log) {
			err = errno;
			syslog(LOG_INFO, "failed to open device %s for read: %s",
			     device, strerror(err));
			errno = err;
		}
		return -1;
	}

	rc = fscanf(fp, "%s", value);
	fclose(fp);

	if (rc != 1) {
		if (log) {
			err = errno;
			syslog(LOG_INFO, "failed to read device %s: %s",
			     device, strerror(err));
			errno = err;
		}
		return -1;
	}

  return 0;
}

static int read_sysfs_raw(char *sysfs_path, char *buffer)
{
	return read_sysfs_raw_internal(sysfs_path, buffer, 1);
}

// Returns 0 for success, or -1 on failures.
static int read_sysfs_int(char *sysfs_path, int *buffer)
{
	int rc;
	char readBuf[PATH_CACHE_SIZE];

	if (sysfs_path == NULL || buffer == NULL) {
		errno = EINVAL;
		return -1;
	}

	rc = read_sysfs_raw(sysfs_path, readBuf);
	if (rc == 0)
	{
		if (strstr(readBuf, "0x") || strstr(readBuf, "0X"))
			sscanf(readBuf, "%x", buffer);
		else
			sscanf(readBuf, "%d", buffer);
	}
	return rc;
}

static int write_sysfs_raw_internal(const char *device, char *value, int log)
{
	FILE *fp;
	int rc, err;

	fp = fopen(device, "w");
	if (!fp) {
		if (log) {
			err = errno;
			syslog(LOG_INFO, "failed to open device %s for write : %s",
			     device, strerror(err));
			errno = err;
		}
		return err;
	}

	rc = fputs(value, fp);
	fclose(fp);

	if (rc < 0) {
		if (log) {
			err = errno;
			syslog(LOG_INFO, "failed to write to device %s", device);
			errno = err;
		}
		return -1;
	}

  return 0;
}

static int write_sysfs_raw(char *sysfs_path, char* buffer)
{
	return write_sysfs_raw_internal(sysfs_path, buffer, 1);
}

// Returns 0 for success, or -1 on failures.
static int write_sysfs_int(char *sysfs_path, int buffer)
{
	int rc;
	char writeBuf[PATH_CACHE_SIZE];

	if (sysfs_path == NULL) {
		errno = EINVAL;
		return -1;
	}

	snprintf(writeBuf, PATH_CACHE_SIZE, "%d", buffer);
	return write_sysfs_raw(sysfs_path, writeBuf);
}


static int read_temp_sysfs(struct sensor_info_sysfs *sensor)
{
	int ret;
	int value;
	char fullpath[PATH_CACHE_SIZE];
	bool use_cache = false;
	int cache_str_len = 0;

	if (sensor == NULL) {
		syslog(LOG_NOTICE, "sensor is null\n");
		return BAD_TEMP;
	}
	// Check if cache is available
	if (sensor->path_cache != NULL) {
		cache_str_len = strlen(sensor->path_cache);
		if (cache_str_len != 0)
			use_cache = true;
	}

	if (use_cache == false) {
		// No cached value yet. Calculate the full path first
		ret = assemble_sysfs_path(sensor->prefix, sensor->suffix, fullpath, sizeof(fullpath));
		if(ret != 0) {
			syslog(LOG_NOTICE, "%s: I2C bus %s not available. Failed reading %s\n", __FUNCTION__, sensor->prefix, sensor->suffix);
			return BAD_TEMP;
		}
		// Update cache, if possible.
		if (sensor->path_cache != NULL)
			snprintf(sensor->path_cache, (PATH_CACHE_SIZE - 1), "%s", fullpath);
		use_cache = true;
	}

	/*
	* By the time control reaches here, use_cache is always true
	* or this function already returned -1. So assume the cache is always on
	*/
	ret = read_sysfs_int(sensor->path_cache, &value);

	/*  Note that Kernel sysfs stub pre-converts raw value in xxxxx format,
	*  which is equivalent to xx.xxx degree - all we need to do is to divide
	*  the read value by 1000
	*/
	if (ret < 0)
		value = ret;
	else
		value = value / 1000;

	if (value < 0) {
		syslog(LOG_ERR, "failed to read temperature bus %s", fullpath);
		return BAD_TEMP;
	}

	usleep(11000);
	return value;
}


static int read_critical_max_temp(void)
{
  int i;
  int temp, max_temp = 0;
  struct board_info_stu_sysfs *info;

	for(i = 0; i < BOARD_INFO_SIZE; i++) {
		info = &board_info[i];
			if(info->critical) {
				temp = read_temp_sysfs(info->critical);
				if(temp != -1) {
					info->critical->temp = temp;
				if(temp > max_temp)
					max_temp = temp;
			}
		}
	}
	//printf("%s: critical: max_temp=%d\n", __func__, max_temp);

	return max_temp;
}

static int alarm_temp_update(void)
{
	int i;
	int temp, max_temp = 0;
	struct board_info_stu_sysfs *info;

	for(i = 0; i < BOARD_INFO_SIZE; i++) {
		info = &board_info[i];
		if(info->alarm) {
			temp = read_temp_sysfs(info->alarm);
			if(temp != -1) {
				info->alarm->temp = temp;
				if(temp >= info->hwarn ||
					((info->hwarn - temp <= ALARM_TEMP_THRESHOLD) && info->warn_count)) {
					if(++info->warn_count >= ALARM_START_REPORT) {
						syslog(LOG_WARNING, "Warning: %s arrived %d C(High Warning: >= %d C)",
							info->name, temp, info->hwarn);
						info->warn_count = 0;
					}
				} else if(temp >= info->lwarn ||
							((info->lwarn - temp <= ALARM_TEMP_THRESHOLD) && info->warn_count)) {
					if(++info->warn_count >= ALARM_START_REPORT) {
						syslog(LOG_WARNING, "Warning: %s arrived %d C(Low Warning: >= %d C)",
							info->name, temp, info->lwarn);
						info->warn_count = 0;
					}
				}
			}
		}
	}
	//printf("%s: alarm: max_temp=%d\n", __func__, max_temp);

	return max_temp;
}

static int calculate_raising_fan_pwm(int temp)
{
	int slope;
	int val;

	if(temp < RAISING_TEMP_LOW) {
		return FAN_LOW;
	} else if(temp >= RAISING_TEMP_LOW && temp < RAISING_TEMP_HIGH) {
		slope = (FAN_HIGH - FAN_LOW) / (RAISING_TEMP_HIGH - RAISING_TEMP_LOW);
		val = FAN_LOW + slope * temp;
		return val;
	} else  {
		return FAN_HIGH;
	}
		return FAN_HIGH;
}
static int calculate_falling_fan_pwm(int temp)
{
	int slope;
	int val;

	if(temp < FALLING_TEMP_LOW) {
		return FAN_LOW;
	} else if(temp >= FALLING_TEMP_LOW && temp < FALLING_TEMP_HIGH) {
		slope = (FAN_HIGH - FAN_LOW) / (FALLING_TEMP_HIGH - FALLING_TEMP_LOW);
		val = FAN_LOW + slope * temp;
		return val;		
	} else  {
		return FAN_HIGH;
	}

	return FAN_HIGH;
}

#ifdef CONFIG_PSU_FAN_CONTROL
#define PSU_FAN_LOW 45
static int calculate_psu_raising_fan_pwm(int temp)
{
	int slope;
	int val;

	if(temp < RAISING_TEMP_LOW) {
		return PSU_FAN_LOW;
	} else if(temp >= RAISING_TEMP_LOW && temp < RAISING_TEMP_HIGH) {
		slope = (FAN_HIGH - PSU_FAN_LOW) / (RAISING_TEMP_HIGH - RAISING_TEMP_LOW);
		val = PSU_FAN_LOW + slope * temp;
		return val;
	} else  {
		return FAN_HIGH;
	}
		return FAN_HIGH;
}
static int calculate_psu_falling_fan_pwm(int temp)
{
	int slope;
	int val;

	if(temp < FALLING_TEMP_LOW) {
		return PSU_FAN_LOW;
	} else if(temp >= FALLING_TEMP_LOW && temp < FALLING_TEMP_HIGH) {
		slope = (FAN_HIGH - PSU_FAN_LOW) / (FALLING_TEMP_HIGH - FALLING_TEMP_LOW);
		val = PSU_FAN_LOW + slope * temp;
		return val;		
	} else  {
		return FAN_HIGH;
	}

	return FAN_HIGH;
}

#endif

/*
 * Fan number here is 0-based
 * Note that 1 means present
 */
static int fan_is_present_sysfs(int fan, struct fan_info_stu_sysfs *fan_info)
{
	int ret;
	char buf[PATH_CACHE_SIZE];
	int rc = 0;

	snprintf(buf, PATH_CACHE_SIZE, "%s/%s", fan_info->prefix, fan_info->present);

	rc = read_sysfs_int(buf, &ret);
	if(rc < 0) {
		syslog(LOG_ERR, "failed to read module present %s node", fan_info->present);
		return -1;
	}

	usleep(11000);

	if (ret != 0) {
		syslog(LOG_ERR, "%s: FAB-%d not present", __func__, fan + 1);
		return 0;
	} else {
		return 1;
	}

	return 0;
}


// Note that the fan number here is 0-based
static int set_fan_sysfs(int fan, int value)
{
	int ret;
	struct fantray_info_stu_sysfs *fantray_info;
	struct fan_info_stu_sysfs *fan_info;

	fantray_info = &fantray_info[fan];
	fan_info = &fantray_info->fan1;
	
	char fullpath[PATH_CACHE_SIZE];

	ret = fan_is_present_sysfs(fan, fan_info);
	if(ret == 0) {
		fantray_info->present = 0; //not preset
		return -1;
	} else if(ret == 1) {
		fantray_info->present = 1;
	} else {
		return -1;
	}

	snprintf(fullpath, PATH_CACHE_SIZE, "%s/%s", fan_info->prefix, fan_info->pwm_prefix);
	ret = write_sysfs_int(fullpath, value);
	if(ret < 0) {
		syslog(LOG_ERR, "failed to set fan %s/%s, value %#x",
		fan_info->prefix, fan_info->pwm_prefix, value);
		return -1;
	}
	usleep(11000);

	return 0;
}

static int write_fan_led_sysfs(int fan, const int color)
{
	int ret;
	char fullpath[PATH_CACHE_SIZE];
	struct fantray_info_stu_sysfs *fantray_info;
	struct fan_info_stu_sysfs *fan_info;
	
	fantray_info = &fantray_info[fan];
	fan_info = &fantray_info->fan1;
	

	ret = fan_is_present_sysfs(fan, fan_info);
	if(ret == 0) {
		fantray_info->present = 0; //not preset
		return -1;
	} else if(ret == 1) {
		fantray_info->present = 1;
	} else {
		return -1;
	}

	snprintf(fullpath, PATH_CACHE_SIZE, "%s/%s", fan_info->prefix, fan_info->fan_led_prefix);
	ret = write_sysfs_int(fullpath, color);
	if(ret < 0) {
		syslog(LOG_ERR, "failed to set fan %s/%s, value %#x",
		fan_info->prefix, fan_info->fan_led_prefix, color);
		return -1;
	}
	usleep(11000);

	return 0;
}


/* Set fan speed as a percentage */
static int write_fan_speed(const int fan, const int value)
{
	int unit;

	unit = value * PWM_UNIT_MAX / 100;

 	return set_fan_sysfs(fan, unit);
}

/* Set up fan LEDs */
static int write_fan_led(const int fan, const int color)
{
	return write_fan_led_sysfs(fan, color);
}

#ifdef CONFIG_PSU_FAN_CONTROL
/* Set PSU fan speed as a percentage */
static int write_psu_fan_speed(const int fan, const int value)
{
	int err;
	int unit;

	unit = value * PWM_UNIT_MAX / 100;

 	err = set_fan_sysfs(TOTAL_FANS, unit);
	err += set_fan_sysfs(TOTAL_FANS + 1, unit);

	return err;
}

/* Set up fan LEDs */
static int write_psu_fan_led(const int fan, const int color)
{
	int err;
	
	err = write_fan_led_sysfs(TOTAL_FANS, color);
	err += write_fan_led_sysfs(TOTAL_FANS, color);

	return err;
}

#endif

static int fan_rpm_to_pct(const struct rpm_to_pct_map *table, const int table_len, int rpm)
{
	int i;
	
	for (i = 0; i < table_len; i++) {
		if (table[i].rpm > rpm) {
			break;
		}
	}

	/*
	 * If the fan RPM is lower than the lowest value in the table,
	 * we may have a problem -- fans can only go so slow, and it might
	 * have stopped.  In this case, we'll return an interpolated
	 * percentage, as just returning zero is even more problematic.
	 */

	if (i == 0) {
		return (rpm * table[i].pct) / table[i].rpm;
	} else if (i == table_len) { // Fell off the top?
		return table[i - 1].pct;
	}
	
	// Interpolate the right percentage value:
	
	int percent_diff = table[i].pct - table[i - 1].pct;
	int rpm_diff = table[i].rpm - table[i - 1].rpm;
	int fan_diff = table[i].rpm - rpm;

	return table[i].pct - (fan_diff * percent_diff / rpm_diff);
}


/*return: 1 OK, 0 not OK*/
int fan_speed_okay(const int fan, const int speed, const int slop)
{
	int ret;
	char buf[PATH_CACHE_SIZE];
	int rc = 0;
	int front_speed, front_pct;
	int rear_speed, rear_pct;
	struct fantray_info_stu_sysfs *fantray_info;
	struct fan_info_stu_sysfs *fan_info;
	
	fantray_info = &fantray_info[fan];
	fan_info = &fantray_info->fan1;

	ret = fan_is_present_sysfs(fan, fan_info);
	if(ret == 0) {
		fantray_info->present = 0; //not preset
		return 0;
	} else if(ret == 1) {
		fantray_info->present = 1;
	}

	snprintf(buf, PATH_CACHE_SIZE, "%s/%s", fan_info->prefix, fan_info->front_fan_prefix);

	rc = read_sysfs_int(buf, &ret);
	if(rc < 0) {
		syslog(LOG_ERR, "failed to read module present %s node", fan_info->present);
		return -1;
	}
	usleep(11000);
	front_speed = rc * 150;
	front_pct = fan_rpm_to_pct(rpm_front_map, FRONT_MAP_SIZE, front_speed);

	memset(buf, 0, PATH_CACHE_SIZE);
	snprintf(buf, PATH_CACHE_SIZE, "%s/%s", fan_info->prefix, fan_info->rear_fan_prefix);

	rc = read_sysfs_int(buf, &ret);
	if(rc < 0) {
		syslog(LOG_ERR, "failed to read module present %s node", fan_info->present);
		return -1;
	}
	rear_speed = rc * 150;
	rear_pct = fan_rpm_to_pct(rpm_rear_map, FRONT_MAP_SIZE, rear_speed);

	rc = (abs(front_speed - speed) * 100 / speed < slop) &&
			(abs(rear_speed - speed) * 100 / speed < slop);
	if(!rc) {
		 syslog(LOG_WARNING, "fan %d front %d (%d%%), rear %d (%d%%), expected %d",
		 	fan, front_speed, front_pct, rear_speed, rear_pct, speed);
	}

	return rc;

}


static int fancpld_watchdog_enable(void)
{
	int ret;
	char fullpath[PATH_CACHE_SIZE];
	
	snprintf(fullpath, PATH_CACHE_SIZE, "%s", FAN_WTD_SYSFS);
	ret = write_sysfs_int(fullpath, 1);
	if(ret < 0) {
		syslog(LOG_ERR, "failed to set fan %s, value 1",
		FAN_WTD_SYSFS);
		return -1;
	}
	usleep(11000);

	return 0;
}

static int system_shutdown(const char *why)
{
	int ret;

	syslog(LOG_EMERG, "Shutting down:  %s", why);

	ret = write_sysfs_int(PSU1_SHUTDOWN_SYSFS, 1);
	if(ret < 0) {
		syslog(LOG_ERR, "failed to set PSU1 shutdown");
		return -1;
	}
	ret = write_sysfs_int(PSU2_SHUTDOWN_SYSFS, 1);
	if(ret < 0) {
		syslog(LOG_ERR, "failed to set PSU2 shutdown");
		return -1;
	}

	stop_watchdog();

	sleep(2);
	exit(2);

	return 0;
}



int main(int argc, char **argv) {
	int critical_temp;
	int old_temp = 0;
	int raising_pwm;
	int falling_pwm;
	struct fantray_info_stu_sysfs *info;
	int fan_speed = FAN_MEDIUM;
	int bad_reads = 0;
	int fan_failure = 0;
	int old_speed;
	int fan_bad[TOTAL_FANS];
	int fan;
	unsigned log_count = 0; // How many times have we logged our temps?
	int prev_fans_bad = 0;
	int shutdown_delay = 0;
#ifdef CONFIG_PSU_FAN_CONTROL
	int psu_old_temp = 0;
	int psu_raising_pwm;
	int psu_falling_pwm;
	int psu_fan_speed = FAN_MEDIUM;
#endif

	// Initialize path cache
	init_path_cache();

	// Start writing to syslog as early as possible for diag purposes.
	openlog("fand", LOG_CONS, LOG_DAEMON);

	daemon(1, 0);

	syslog(LOG_DEBUG, "Starting up;  system should have %d fans.", TOTAL_FANS);

	/* Start watchdog in manual mode */
	start_watchdog(0);

	/* Set watchdog to persistent mode so timer expiry will happen independent
	* of this process's liveliness. */
	set_persistent_watchdog(WATCHDOG_SET_PERSISTENT);

	fancpld_watchdog_enable();

	sleep(5);  /* Give the fans time to come up to speed */

	while (1) {
		/* Read sensors */
		critical_temp = read_critical_max_temp();
		alarm_temp_update();

		if (critical_temp == BAD_TEMP) {
			if(bad_reads++ >= 10) {
				if(critical_temp == BAD_TEMP) {
					syslog(LOG_ERR, "Critical Temp read error!");
				}
				bad_reads = 0;
			}
		}

		if (log_count++ % REPORT_TEMP == 0) {
		  syslog(LOG_DEBUG,
		         "critical temp %d, fan speed %d",
		         critical_temp, fan_speed);
		}

		/* Protection heuristics */
		if(critical_temp > SYSTEM_LIMIT) {
			for (fan = 0; fan < TOTAL_FANS; fan++) {
				write_fan_speed(fan, fan_speed);
			}
			if(shutdown_delay++ > SHUTDOWN_DELAY_TIME)
				system_shutdown("Critical temp limit reached");
		}

		/*
		 * Calculate change needed -- we should eventually
		 * do something more sophisticated, like PID.
		 *
		 * We should use the intake temperature to adjust this
		 * as well.
		 */

		/* Other systems use a simpler built-in table to determine fan speed. */
		raising_pwm = calculate_raising_fan_pwm(critical_temp);
		falling_pwm = calculate_falling_fan_pwm(critical_temp);
		if(old_temp <= critical_temp) {
			/*raising*/
			if(raising_pwm >= fan_speed) {
				fan_speed = raising_pwm;
			}
		} else {
			/*falling*/
			if(falling_pwm <= fan_speed ) {
				fan_speed = falling_pwm;
			}
		}
		old_temp = critical_temp;
#ifdef CONFIG_PSU_FAN_CONTROL
		psu_raising_pwm = calculate_psu_raising_fan_pwm(critical_temp);
		psu_falling_pwm = calculate_psu_falling_fan_pwm(critical_temp);
		if(psu_old_temp <= critical_temp) {
			/*raising*/
			if(psu_raising_pwm >= psu_fan_speed) {
				psu_fan_speed = psu_raising_pwm;
			}
		} else {
			/*falling*/
			if(psu_falling_pwm <= psu_fan_speed ) {
				psu_fan_speed = psu_falling_pwm;
			}
		}
		psu_old_temp = critical_temp;
#endif

		/*
		 * Update fans only if there are no failed ones. If any fans failed
		 * earlier, all remaining fans should continue to run at max speed.
		 */
		if (fan_failure == 0) {
			syslog(LOG_NOTICE,
				"critical temp %d, fan speed %d",
				critical_temp, fan_speed);
			syslog(LOG_NOTICE,
				"Fan speed changing from %d to %d",
				old_speed, fan_speed);
			for (fan = 0; fan < TOTAL_FANS; fan++) {
				write_fan_speed(fan, fan_speed);
			}
#ifdef CONFIG_PSU_FAN_CONTROL
			write_psu_fan_speed(fan, psu_fan_speed);
#endif
		}

		
		/*
		 * Wait for some change.  Typical I2C temperature sensors
		 * only provide a new value every second and a half, so
		 * checking again more quickly than that is a waste.
		 *
		 * We also have to wait for the fan changes to take effect
		 * before measuring them.
		 */

		sleep(5);

	    /* Check fan RPMs */

	    for (fan = 0; fan < TOTAL_FANS; fan++) {
			/*
			* Make sure that we're within some percentage
			* of the requested speed.
			*/
			if (fan_speed_okay(fan, fan_speed, FAN_FAILURE_OFFSET)) {
				if (fan_bad[fan] >= FAN_FAILURE_THRESHOLD) {
					write_fan_led(fan, FAN_LED_GREEN);
					syslog(LOG_CRIT, "Fan %d has recovered", fan);
				}
				fan_bad[fan] = 0;
			} else {
				fan_bad[fan]++;
			}
		}

		fan_failure = 0;
		for (fan = 0; fan < TOTAL_FANS; fan++) {
			if (fan_bad[fan] >= FAN_FAILURE_THRESHOLD) {
				fan_failure++;
				write_fan_led(fan, FAN_LED_RED);
			}
		}
		if (fan_failure > 0) {
			if (prev_fans_bad != fan_failure) {
				syslog(LOG_CRIT, "%d fans failed", fan_failure);
			}
			fan_speed = FAN_MAX;
			for (fan = 0; fan < TOTAL_FANS; fan++) {
				write_fan_speed(fan, fan_speed);
			}
#ifdef CONFIG_PSU_FAN_CONTROL
			write_psu_fan_speed(fan, fan_speed);
#endif
			old_speed = fan_speed;
		} else if(prev_fans_bad != 0 && fan_failure == 0){
			old_speed = 0;
		} else {			
			old_speed = fan_speed;
		}
		/* Suppress multiple warnings for similar number of fan failures. */
		prev_fans_bad = fan_failure;

		/* if everything is fine, restart the watchdog countdown. If this process
		 * is terminated, the persistent watchdog setting will cause the system
		 * to reboot after the watchdog timeout. */
		kick_watchdog();
		usleep(11000);
	}

	return 0;
}

