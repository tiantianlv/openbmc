#!/bin/sh
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

# Bus 0
i2c_device_add 0 0x0d syscpld  #CPLD

# Bus 1
i2c_device_add 1 0x50 24c02  #CPU EEPROM

# Bus 2
i2c_device_add 2 0x53 24c64  #EEPROM

# Bus 3
i2c_device_add 3 0x50 24c64  #EEPROM

# Bus 4
i2c_device_add 4 0x12 ir3595  #IR3595
i2c_device_add 4 0x70 ir3584  #IR3584

# Bus 5
#i2c_device_add 5 0x36 fpga    #FPGA

# Bus 6
i2c_device_add 6 0x50 psu_fru #PSU1 FRU
i2c_device_add 6 0x51 psu_fru #PSU2 FRU
i2c_device_add 6 0x58 dps1100 #PSU1 PMBUS
i2c_device_add 6 0x59 dps1100 #PSU2 PMBUS

# Bus 7
i2c_device_add 7 0x49 tmp75 #sensors
i2c_device_add 7 0x4a tmp75
i2c_device_add 7 0x4b tmp75
i2c_device_add 7 0x4c tmp75
i2c_device_add 7 0x4d tmp75
i2c_device_add 7 0x4e tmp75

# Bus 16 Fan EEPROM
i2c_device_add 16 0x50 24c64
i2c_device_add 17 0x50 24c64
i2c_device_add 19 0x50 24c64
i2c_device_add 20 0x50 24c64

# run sensors.config set command
sensors -s
