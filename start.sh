#!/bin/sh

cd /root/ryf-flowers
devname=$(ls /dev/ttyACM* | tail -1)
echo "Using $devname"
PULSE_COOKIE=/root/.config/pulse/cookie ./flowers $devname
