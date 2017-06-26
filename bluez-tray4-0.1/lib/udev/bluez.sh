#!/bin/sh
[ $1 ] || exit
/usr/sbin/bluetoothd -u
until [ -f /tmp/dsp ]; do /bin/sleep 1; done
export DISPLAY=$(/bin/cat /tmp/dsp)
/usr/bin/bluez-tray "$1" "$2"
