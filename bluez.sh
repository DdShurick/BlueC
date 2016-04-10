#!/bin/sh
[ $1 ] || exit
/usr/sbin/bluetoothd -u
until [ -f /tmp/X ]; do sleep 1; done
/usr/bin/bluez-tray "$1"
