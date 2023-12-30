#!/bin/bash
ifconfig eth0 addr `./nfeth-config --get-atari-ip eth0` netmask `./nfeth-config --get-netmask eth0` up
route add default eth0 gw `./nfeth-config --get-host-ip eth0`
