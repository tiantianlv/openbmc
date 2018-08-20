#!/bin/bash

SYSCPLD_SYSFS_DIR="/sys/bus/i2c/devices/i2c-0/0-000d"
USRV_STATUS_SYSFS="${SYSCPLD_SYSFS_DIR}/come_status"
PWR_BTN_SYSFS="${SYSCPLD_SYSFS_DIR}/cb_pwr_btn_n"
PWR_RESET_SYSFS="${SYSCPLD_SYSFS_DIR}/come_rst_n"
SYSLED_CTRL_SYSFS="${SYSCPLD_SYSFS_DIR}/sysled_ctrl"
SYSLED_SEL_SYSFS="${SYSCPLD_SYSFS_DIR}/sysled_select"


wedge_power() {
	if [ "$1" == "on" ]; then
		echo 0 > $PWR_BTN_SYSFS
		sleep 1
		echo 1 > $PWR_BTN_SYSFS
	elif [ "$1" == "off" ]; then
		echo 0 > $PWR_BTN_SYSFS
		sleep 5
		echo 1 > $PWR_BTN_SYSFS
	elif [ "$1" == "reset" ]; then
		echo 0 > $PWR_RESET_SYSFS
		sleep 10
	else
		echo -n "Invalid parameter"
		return 1
	fi
	wedge_is_us_on
	return $?
}

wedge_is_us_on() {
    local val n retries prog
    if [ $# -gt 0 ]; then
        retries="$1"
    else
        retries=1
    fi
    if [ $# -gt 1 ]; then
        prog="$2"
    else
        prog=""
    fi
    if [ $# -gt 2 ]; then
        default=$3              # value 0 means defaul is 'ON'
    else
        default=1
    fi
    ((val=$(cat $USRV_STATUS_SYSFS 2> /dev/null | head -n 1)))
	if [ -z "$val" ]; then
        return $default
	elif [ $val -eq 15 ]; then
        return 0            # powered on
    elif [ $val -eq 0 ]; then
        return 1
	else
		return 2
    fi
}

sys_led_usage() {
	echo "option: "
	echo "<green| yellow| mix| off> #LED color select"
	echo "<on| off| fast| slow>     #LED turn on, off or blink"
	echo 
}

sys_led_show() {
	local val
	sel=$(cat $SYSLED_SEL_SYSFS 2> /dev/null | head -n 1)
	ctrl=$(cat $SYSLED_CTRL_SYSFS 2> /dev/null | head -n 1)

	case "$sel" in
		0x0)
			val="green and yellow"
			;;
		0x1)
			val="green"
			;;
		0x2)
			val="yellow"
			;;
		0x3)
			val="off"
			;;
	esac
	echo -n "LED: $val "
	
	case "$ctrl" in
		0x0)
			val="on"
			;;
		0x1)
			val="1HZ blink"
			;;
		0x2)
			val="4HZ blink"
			;;
		0x3)
			val="off"
			;;
	esac
	echo "$val"
}

sys_led() {
	if [ $# -lt 2 ]; then
		sys_led_show
		return 0
	fi
	if [ "$1" == "green" ]; then
		echo 0x1 > $SYSLED_SEL_SYSFS
	elif [ "$1" == "yellow" ]; then
		echo 0x2 > $SYSLED_SEL_SYSFS
	elif [ "$1" == "mix" ]; then
		echo 0x0 > $SYSLED_SEL_SYSFS
	elif [ "$1" == "off" ]; then
		echo 0x3 > $SYSLED_SEL_SYSFS
	else
		sys_led_usage
		return 1
	fi

	if [ "$2" == "on" ]; then
		echo 0x0 > $SYSLED_CTRL_SYSFS
	elif [ "$2" == "fast" ]; then
		echo 0x2 > $SYSLED_CTRL_SYSFS
	elif [ "$2" == "slow" ]; then
		echo 0x1 > $SYSLED_CTRL_SYSFS
	elif [ "$2" == "off" ]; then
		echo 0x3 > $SYSLED_CTRL_SYSFS
	else
		sys_led_usage
		return 1
	fi
}

board_type() {
	echo 'Fishbone'
	#echo 'Phalanx'
}
