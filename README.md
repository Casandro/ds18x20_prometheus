# ds18x20_prometheus

A simple service for the Raspberry PI. Which exports temperature sensors exported by the Linux [`w1_therm`}(https://www.kernel.org/doc/html/latest/w1/w1-generic.html) drivers. Those include [DS18B20](https://www.maximintegrated.com/en/products/sensors/DS18B20.html), [DS18S20](https://www.maximintegrated.com/en/products/sensors/DS18S20.html), [DS1822](https://www.maximintegrated.com/en/products/sensors/DS1822.html) and [MAX31850](https://www.maximintegrated.com/en/products/sensors/MAX31850.html).

# Usage:

`ds18s20_prometheus <port-number> <id=label>... `

The label allows you to attach human readable names to your sensors.

To find out what sensors you have attached to your Raspberry PI, look for the filesystem interface of the driver.

```
> ls /sys/bus/w1/devices/*/temperature
```

The sensor ID is the part of the path between `devices` and `temperature`. You can label the sensors my appending pairs of `id=label` tuples to the command line.


# Preparing your raspberry pi

Add the device tree overlay to your /boot/config.txt
```
dtoverlay=w1-gpio,gpiopin=4,pullup=on
```
replace the gpiopin=4 with the number of the gpiopin you use. I think the pullup parameter adds an integrated pull-up.


