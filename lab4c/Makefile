#NAME: Naim Ayat
#EMAIL: naimayat@ucla.edu
#ID: 000000000

default:
	make build

build: lab4c_tcp.c lab4c_tls.c
	cc -Wall -Wextra -lm -lmraa -pthread -o lab4c_tcp lab4c_tcp.c
	cc -Wall -Wextra -lm -lmraa -pthread -o lab4c_tls lab4c_tls.c -lssl -lcrypto

dist: README lab4c_tls.c lab4c_tcp.c Makefile
	tar -cvzf lab4c-000000000.tar.gz lab4c_tcp.c lab4c_tls.c README Makefile

check: 
	make
	make clean

clean: 
	rm -f lab4c-000000000.tar.gz lab4c_tcp lab4c_tls

