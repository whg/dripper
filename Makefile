all:
	gcc -std=c99 -Wall -o loader loader.c -lprussdrv -Iinclude
	pasm -b transfer.p

test:
	gcc -std=c99 -Wall -o loader loader.c -lprussdrv
	pasm -b loader_test.p

dto:
	dtc -O dtb -I dts -o /lib/firmware/DRIPER-00A0.dtbo -b 0 -@ DRIPER-00A0.dts
