#!/bin/sh
[ $1 ] || exit
/usr/sbin/bluetoothd -u
while [ -z $DISPLAY ]; do /bin/sleep 1; done
export DISPLAY=:0
/usr/bin/bluez-tray "$1"
