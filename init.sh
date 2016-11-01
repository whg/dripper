#/bin/bash
slots=/sys/devices/bone_capemgr.9/slots
echo DRIPPER >$slots && echo "We're ready to go!"