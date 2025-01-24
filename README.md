# ds18x20_prometheus

A simple service for the Raspberry PI.

# Usage:

`ds18s20_prometheus` <port-number> <id=label>...

The label allows you to attach human readable names to your sensors.

To find out what sensors you have attached to your Raspberry PI, look for the filesystem interface of the driver.

```
> ls /sys/bus/w1/devices/*/temperature
/sys/bus/w1/devices/28-6137550a6461/temperature  /sys/bus/w1/devices/28-9003550a6461/temperature  /sys/bus/w1/devices/28-a967550a6461/temperature
/sys/bus/w1/devices/28-6c05550a6461/temperature  /sys/bus/w1/devices/28-9d0a550a6461/temperature
```

The `28-xxxxxxxxxx` part of the path is the sensor ID. You can attach a label to it, by giving it as a parameter to the command line.

# Preparing your raspberry pi

Add the device tree overlay to your /boot/config.txt
```
dtoverlay=w1-gpio,gpiopin=4,pullup=on
```
replace the gpiopin=4 with the number of the gpiopin you use. I thin the pullup parameter adds an integrated pull-up.


