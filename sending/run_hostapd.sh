#!/bin/bash
cd "$(dirname "$0")"

sudo wpa_cli terminate
sudo ip addr add 192.168.1.2/24 broadcast 192.168.1.255 dev wlp7s0
sudo hostapd ./hostapd.conf #-dd
