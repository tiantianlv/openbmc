#!/bin/bash
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
. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

if /usr/local/bin/boot_info.sh |grep "Slave Flash" ; then
    sys_led yellow on #sysled light yellow
else
    sys_led green on #sysled light green
fi

#Set BMC login timeout
echo "TMOUT=300" >> /etc/profile
echo "export TMOUT" >> /etc/profile

