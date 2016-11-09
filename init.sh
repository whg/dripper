#/bin/bash
slots=/sys/devices/bone_capemgr.9/slots
echo DRIPPER >$slots && echo -n "Device overlay inserted, "
sleep 3
rmmod uio_pruss
sleep 1
modprobe uio_pruss extram_pool_sz=0x80000 && echo "memory expanded"
echo "We're ready to go!"