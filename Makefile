EXEC=loader
all:
	gcc -std=c99 -Wall -o EXEC loader.c -lprussdrv -Iinclude
	pasm -b transfer.p

test:
	gcc -std=c99 -Wall -o EXEC loader.c -lprussdrv
	pasm -b loader_test.p

dto:
	dtc -O dtb -I dts -o /lib/firmware/DRIPPER-00A0.dtbo -b 0 -@ DRIPPER-00A0.dts

install:
	ln -fs $(shell pwd)/${EXEC} /usr/local/bin/
	ln -fs $(shell pwd)/include/vol/scripts/inspect.py /usr/local/bin/
	ln -fs $(shell pwd)/include/vol/scripts/inspect.py /usr/local/bin/
	ln -fs $(shell pwd)/include/vol/scripts/generate.py /usr/local/bin/

