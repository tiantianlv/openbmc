#!/bin/sh
#
# Copyright 2018-present Celestica. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

. /usr/local/bin/openbmc-utils.sh

set_value() {
	echo ${4} > /sys/bus/i2c/devices/i2c-${1}/${1}-00${2}/${3} 2> /dev/null
}

set_hwmon_value() {
	echo ${5} > /sys/bus/i2c/devices/i2c-${1}/${1}-00${2}/hwmon/hwmon${3}/${4} 2> /dev/null
}

get_hwmon_id() {
	path="/sys/bus/i2c/devices/i2c-${1}/${1}-00${2}/"
	str=$(find $path -name "$3")
	id=$(echo $str | awk -F 'hwmon' '{print $3}' | awk -F '/' '{print $1}')
	if [ $id ]; then
		if [ "$id" -gt 0 ] 2>/dev/null; then
			echo $id
		else
			echo 0
		fi
		return 0
	fi
	echo 0
}

board_type=$(board_type)

#func    bus addr node val
#IR38060
set_value 4 43 in0_min 950
set_value 4 43 in0_max 1100
set_value 4 43 curr1_min 0
set_value 4 43 curr1_max 6000
set_value 4 43 in0_label "XP1R0V_FPGA Voltage"
set_value 4 43 curr1_label "XP1R0V_FPGA Current"

set_value 17 47 in0_min 1160
set_value 17 47 in0_max 1240
set_value 17 47 curr1_min 0
set_value 17 47 curr1_max 6000
set_value 17 47 in0_label "XP1R2V Voltage"
set_value 17 47 curr1_label "XP1R2V Current"

#IR38062
set_value 4 49 in0_min 3130
set_value 4 49 in0_max 3470
set_value 4 49 curr1_min 0
set_value 4 49 curr1_max 15000
set_value 4 49 in0_label "XP3R3V Voltage"
set_value 4 49 curr1_label "XP3R3V Current"

#IR3595
set_value 16 12 in0_min 730
set_value 16 12 in0_max 1020
set_value 16 12 curr1_min 0
set_value 16 12 curr1_max 211700
set_value 16 12 in0_label "VDDCORE Voltage"
set_value 16 12 curr1_label "VDDCORE Current"

#IR3584
set_value 18 70 vout_mode -8
set_value 18 70 in0_min 3130
set_value 18 70 in0_max 3470
set_value 18 70 curr1_min 0
set_value 18 70 curr1_max 31000
set_value 18 70 in0_label "Switch_3V3 Voltage"
set_value 18 70 curr1_label "Switch_3V3 Current"

set_value 18 71 in0_min 720
set_value 18 71 in0_max 880
set_value 18 71 curr1_min 0
set_value 18 71 curr1_max 28000
set_value 18 71 in0_label "Switch_0V8 Voltage"
set_value 18 71 curr1_label "Switch_0V8 Current"

set_value 4 15 in0_min 1600
set_value 4 15 in0_max 1950
set_value 4 15 curr1_min 0
set_value 4 15 curr1_max 43000
set_value 4 15 in0_label "XP1R82V Voltage"
set_value 4 15 curr1_label "XP1R82V Current"

set_value 4 16 in0_min 1000
set_value 4 16 in0_max 1100
set_value 4 16 curr1_min 0
set_value 4 16 curr1_max 14000
set_value 4 16 in0_label "XP1R05V Voltage"
set_value 4 16 curr1_label "XP1R05V Current"


#Fan1-4
set_value 8 0d fan1_min 1000
set_value 8 0d fan1_max 26000
set_value 8 0d fan2_min 1000
set_value 8 0d fan2_max 30000
set_value 8 0d fan3_min 1000
set_value 8 0d fan3_max 26000
set_value 8 0d fan4_min 1000
set_value 8 0d fan4_max 30000
set_value 8 0d fan5_min 1000
set_value 8 0d fan5_max 26000
set_value 8 0d fan6_min 1000
set_value 8 0d fan6_max 30000
set_value 8 0d fan7_min 1000
set_value 8 0d fan7_max 26000
set_value 8 0d fan8_min 1000
set_value 8 0d fan8_max 30000

#PSU1
#add it to sensors.config
val=$(get_hwmon_id 24 58 in1_min)
if [ "$val" -gt "0" ] ; then
	set_hwmon_value 24 58 $val in1_min 90000
	set_hwmon_value 24 58 $val in1_max 264000
	set_hwmon_value 24 58 $val in2_min 11640
	set_hwmon_value 24 58 $val in2_max 12360
	set_hwmon_value 24 58 $val fan1_min 1000
	set_hwmon_value 24 58 $val fan1_max 30000
	set_hwmon_value 24 58 $val temp1_max_hyst 60000
	set_hwmon_value 24 58 $val temp1_max 70000
	set_hwmon_value 24 58 $val temp2_max_hyst 60000
	set_hwmon_value 24 58 $val temp2_max 70000
	set_hwmon_value 24 58 $val power1_max 1222000000
	set_hwmon_value 24 58 $val power2_max 1100000000
	set_hwmon_value 24 58 $val curr1_min 0
	set_hwmon_value 24 58 $val curr1_max 7000
	set_hwmon_value 24 58 $val curr2_min 0
	set_hwmon_value 24 58 $val curr2_max 90000
fi
#PSU2
#add it to sensors.config
val=$(get_hwmon_id 25 59 in1_min)
if [ "$val" -gt "0" ] ; then
	set_hwmon_value 25 59 $val in1_min 90000
	set_hwmon_value 25 59 $val in1_max 264000
	set_hwmon_value 25 59 $val in2_min 11640
	set_hwmon_value 25 59 $val in2_max 12360
	set_hwmon_value 25 59 $val fan1_min 1000
	set_hwmon_value 25 59 $val fan1_max 30000
	set_hwmon_value 25 59 $val temp1_max_hyst 60000
	set_hwmon_value 25 59 $val temp1_max 70000
	set_hwmon_value 25 59 $val temp2_max_hyst 60000
	set_hwmon_value 25 59 $val temp2_max 70000
	set_hwmon_value 25 59 $val power1_max 1222000000
	set_hwmon_value 25 59 $val power2_max 1100000000
	set_hwmon_value 25 59 $val curr1_min 0
	set_hwmon_value 25 59 $val curr1_max 7000
	set_hwmon_value 25 59 $val curr2_min 0
	set_hwmon_value 25 59 $val curr2_max 90000
fi

#temp
val=$(get_hwmon_id 39 48 temp1_max)
if [ "$val" -gt "0" ] ; then
	set_hwmon_value 39 48 $val temp1_max 70000
	set_hwmon_value 39 48 $val temp1_max_hyst 60000
fi
val=$(get_hwmon_id 39 49 temp1_max)
if [ "$val" -gt "0" ] ; then
	set_hwmon_value 39 49 $val temp1_max 70000
	set_hwmon_value 39 49 $val temp1_max_hyst 60000
fi


# run sensors.config set command
sensors -s
sleep 3

#run power monitor
echo "Start Power monitor"
/usr/local/bin/power_monitor.py &
