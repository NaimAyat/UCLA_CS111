#NAME: Naim Ayat
#EMAIL: naimayat@ucla.edu
#ID: 000000000
default:
	make build

build: lab3a.c
	cc -Wall -Wextra -o lab3a lab3a.c

dist: lab3a.c Makefile README ext2_fs.h
	tar -cvzf lab3a-000000000.tar.gz lab3a.c ext2_fs.h README Makefile

check: lab3a.c
	make
	make clean

clean:
	rm -f lab3a-000000000.tar.gz lab3a
