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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://rest-api-1/rest.py \
    file://rest-api-1/rest_fruid.py \
    file://rest-api-1/rest_led.py \
    file://rest-api-1/rest_sensors.py \
    file://board_endpoint.py \
    file://boardroutes.py \
    file://board_setup_routes.py \
    file://common_endpoint.py \
    file://common_setup_routes.py \
    file://rest_utils.py \
    "

binfiles1 += " \
    rest.py \
    rest_fruid.py \
    rest_led.py \
    rest_sensors.py \
    "

binfiles += " \
    board_endpoint.py \
    board_setup_routes.py \
    boardroutes.py \
    common_endpoint.py \
    common_setup_routes.py \
    rest_utils.py \
    "
