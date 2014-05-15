#/bin/bash
slots=/sys/devices/bone_capemgr.9/slots
echo DRIPPER >$slots && echo "We're ready to go!"
rmmod uio_pruss
modprobe uio_pruss extram_pool_sz=0x80000