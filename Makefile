all:
	gcc -Wall -o loader loader.c -lprussdrv
	pasm -b transfer.p

test:
	gcc -Wall -o loader loader.c -lprussdrv
	pasm -b loader_test.p
