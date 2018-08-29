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

board_type=$(board_type)
if [ "$board_type" = "Phalanx" ]; then
	mv /etc/sensors.d/phalanx.conf /etc/sensors.d/as58xx-cl.conf
	rm /etc/sensors.d/fishbone.conf
else
	mv /etc/sensors.d/fishbone.conf /etc/sensors.d/as58xx-cl.conf
	rm /etc/sensors.d/phalanx.conf
fi

#func    bus addr node val
#IR38060
set_value 4 43 in0_min 950
set_value 4 43 in0_max 1050
set_value 4 43 curr1_min 0
set_value 4 43 curr1_max 3000

set_value 17 47 in0_min 1164
set_value 17 47 in0_max 1236
set_value 17 47 curr1_min 0
set_value 17 47 curr1_max 1700

#IR38062
set_value 4 49 in0_min 3135
set_value 4 49 in0_max 3465
set_value 4 49 curr1_min 0
set_value 4 49 curr1_max 10000

#IR3595
set_value 16 12 in0_min 735
set_value 16 12 in0_max 1020
set_value 16 12 curr1_min 0
set_value 16 12 curr1_max 211700

#IR3584
set_value 18 70 in0_min 3135
set_value 18 70 in0_max 3465
set_value 18 70 curr1_min 0
set_value 18 70 curr1_max 31000

#Fan1-4
set_value 8 0d fan1_min 1000
set_value 8 0d fan1_max 26000
set_value 8 0d fan2_min 1000
set_value 8 0d fan2_max 23000
set_value 8 0d fan3_min 1000
set_value 8 0d fan3_max 26000
set_value 8 0d fan4_min 1000
set_value 8 0d fan4_max 23000
set_value 8 0d fan5_min 1000
set_value 8 0d fan5_max 26000
set_value 8 0d fan6_min 1000
set_value 8 0d fan6_max 23000
set_value 8 0d fan7_min 1000
set_value 8 0d fan7_max 26000
set_value 8 0d fan8_min 1000
set_value 8 0d fan8_max 23000

#PSU1
#add it to sensors.config
#PSU2
#add it to sensors.config

if [ "$board_type" = "Phalanx" ]; then
set_value 8 0d fan9_min 1000
set_value 8 0d fan9_max 26000
set_value 8 0d fan10_min 1000
set_value 8 0d fan10_max 23000
fi


# run sensors.config set command
sensors -s
sleep 3

#run power monitor
echo "Start Power monitor"
/usr/local/bin/power_monitor.py &
