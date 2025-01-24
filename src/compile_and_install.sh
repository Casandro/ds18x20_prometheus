#!/bin/bash

gcc -O3 -Wall -Wextra ds18x20_prometheus.c -pthread -o ds18x20_prometheus || exit
sudo systemctl stop ds18x20_prometheus
sudo cp ds18x20_prometheus /usr/local/bin/
sudo cp ds18x20_prometheus.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable ds18x20_prometheus
sudo systemctl start ds18x20_prometheus
