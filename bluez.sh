#!/bin/sh
[ $1 ] || exit

until [ -f /tmp/dsp ]; do /bin/sleep 1; done

export DISPLAY=$(/bin/cat /tmp/dsp)
export $(/bin/grep LANG= /etc/profile)
export LIBASOUND_THREAD_SAFE=0
/usr/libexec/bluetooth/bluetoothd &
/usr/libexec/bluetooth/obexd &
/usr/bin/bluealsa &
/usr/bin/bluez-tray "$@" &
