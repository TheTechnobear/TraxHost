#!/bin/sh


# disable sythor start script
mv /etc/init.d/S90synthor /etc/init.d/D90synthor 
# enable perccmd start script
mv S90perccmd /etc/init.d/S90perccmd


# mv the install script back to it original name
mv SYNTHOR install.sh
# reinstate the original name of the SYNTHOR
mv SYNTHOR.APP SYNTHOR

sync 
sleep 1
sync
reboot
