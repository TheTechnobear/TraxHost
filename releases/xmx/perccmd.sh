#!/bin/sh

echo 1 > /proc/irq/27/smp_affinity # dwc2_hsotg:usb1 ff400000.usb
echo 1 > /proc/irq/28/smp_affinity # ehci_hcd:usb2

echo 11 >  /sys/devices/platform/ff560000.acodec/rk3308-acodec-dev/dac_output

export LD_LIBRARY_PATH=.

cd /media/BOOT
./PercCmd

