# Copyright 2015-present Facebook. All rights reserved.
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

from openbmc_gpio_table import BoardGPIO

board_gpio_table_v1 = [
    BoardGPIO('GPIOA6', 'BMC_MDC'),
    BoardGPIO('GPIOA7', 'BMC_MDIO'),
    BoardGPIO('GPIOF2', 'BMC_EEPROM_SEL'),
    BoardGPIO('GPIOF3', 'BMC_EEPROM_SS'),
    BoardGPIO('GPIOF4', 'BMC_EEPROM_MISO'),
    BoardGPIO('GPIOF5', 'BMC_EEPROM_MOSI'),
    BoardGPIO('GPIOF6', 'BMC_EEPROM_SCK'),
    BoardGPIO('GPIOAA0', 'BMC_FPGA_GPIO7'),
    BoardGPIO('GPIOAA1', 'BMC_FPGA_GPIO6'),
    BoardGPIO('GPIOAA2', 'BMC_FPGA_GPIO4'),
    BoardGPIO('GPIOAA3', 'BMC_FPGA_GPIO5'),
    BoardGPIO('GPIOAA4', 'BMC_JTAG_TDO'),
    BoardGPIO('GPIOAA5', 'BMC_JTAG_TDI'),
    BoardGPIO('GPIOAA6', 'BMC_JTAG_TMS'),
    BoardGPIO('GPIOAA7', 'BMC_JTAG_TCK'),
    BoardGPIO('GPIOAB0', 'EMMC_RST_N'),
    BoardGPIO('GPIOE0', 'BMC_SPI_WP0_N'),
    BoardGPIO('GPIOE1', 'BMC_SPI_WP1_N'),
    BoardGPIO('GPIOE4', 'BIOS_UPGRADE_CPLD'),
    BoardGPIO('GPIOE5', 'BIOS_MUX_SWITCH'),
    BoardGPIO('GPIOE6', 'BMC_GPIOD6'),
    BoardGPIO('GPIOE7', 'BMC_GPIOD7'),
    BoardGPIO('GPIOG4', 'BMC_I2C_ALT1'),
    BoardGPIO('GPIOG5', 'BMC_I2C_ALT2'),
    BoardGPIO('GPIOG6', 'BMC_I2C_ALT3'),
    BoardGPIO('GPIOG7', 'BMC_I2C_ALT4'),
    BoardGPIO('GPIOJ1', 'BMC_RST_OUT'),
    BoardGPIO('GPIOL0', 'RSVD_NCTS1'),
    BoardGPIO('GPIOL1', 'RSVD_NDCD1'),
    BoardGPIO('GPIOL2', 'RSVD_NDSR1'),
    BoardGPIO('GPIOL3', 'RSVD_NRI1'),
    BoardGPIO('GPIOL4', 'RSVD_NDTR1'),
    BoardGPIO('GPIOL5', 'RSVD_NRTS1'),
    BoardGPIO('GPIOM4', 'BMC_FPGA_GPIO1'),
    BoardGPIO('GPIOM5', 'BMC_FPGA_GPIO2'),
    BoardGPIO('GPIOS2', 'COME_12V_EN_B'),
    BoardGPIO('GPIOS3', '5V_EN_B'),
    BoardGPIO('GPIOY0', 'CB_SUS_S3_N'),
    BoardGPIO('GPIOY1', 'CB_SUS_S5_N'),
    BoardGPIO('GPIOY2', 'SIOPWREQ'),
    BoardGPIO('GPIOY3', 'SIOONCTRL'),
    BoardGPIO('GPIOZ0', 'RESET_BTN_N'),
    BoardGPIO('GPIOZ1', 'CB_PWR_OK'),
]
