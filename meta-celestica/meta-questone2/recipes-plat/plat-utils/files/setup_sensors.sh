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

#func    bus addr node val
#IR3595
set_value 4 12 in0_min 500
set_value 4 12 in0_max 2000
set_value 4 12 curr1_min 0
set_value 4 12 curr1_max 100000

#IR3584
set_value 4 70 in0_min 500
set_value 4 70 in0_max 2000
set_value 4 70 curr1_min 0
set_value 4 70 curr1_max 100000

#PSU1
#add it to sensors.config
#PSU2
#add it to sensors.config


# run sensors.config set command
sensors -s
sleep 3

#run power monitor
echo "Start Power monitor"
/usr/local/bin/power_monitor.py &
