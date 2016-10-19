all:
	gcc -Wall -o loader loader.c -lprussdrv

test:
	gcc -Wall -o loader loader.c -lprussdrv
	pasm -b loader_test.p
