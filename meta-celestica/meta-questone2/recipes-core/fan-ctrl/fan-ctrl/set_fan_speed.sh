#!/bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
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
#
usage() {
    echo "Usage: $0 <PERCENT (0..100)> <Fan Unit (1..4)> " >&2
}

FAN_DIR=/sys/bus/i2c/devices/i2c-0/0-000d/

set -e

if [ "$#" -ne 2 ] && [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

if [ "$#" -eq 1 ]; then
    FANS="1 2 3 4"
else
    FANS="$2"
fi

# Convert the percentage to pwm
unit=$(( ( $1 * 255 ) / 100 ))

for fan in $FANS; do
    pwm="${FAN_DIR}/fan${fan}_pwm"
    echo "$unit" > $pwm
    echo "Successfully set fan ${fan} speed to $1%"
done
