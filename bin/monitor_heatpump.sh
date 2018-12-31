#!/usr/bin/env bash

# Enable raw mode (don't do linux's TTY processing)
stty -F /dev/ttyUSB0 raw

# Disable echo when sending data
stty -F /dev/ttyUSB0 -echo -echoe -echok

# 2400 baud 8E1
stty -F /dev/ttyUSB0 2400 cs8 -cstopb parenb -parodd

od -v -t x1 < /dev/ttyUSB0
