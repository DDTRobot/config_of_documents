#!/bin/bash
find /etc/wpa_supplicant/ -type f -name "*wpa_supplicant-nl80211-wlan0.conf*" -exec rm {} \;
gcc -o wifi-app wifi-app.c
cp wifi-app /usr/bin/
systemctl restart systemd-networkd
systemctl restart NetworkManager
# cp wifi-app.service /etc/systemd/system/
# systemctl daemon-reload
# systemctl enable wifi-app.service
# systemctl restart wifi-app.service