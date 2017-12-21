#NAME: Naim Ayat
#EMAIL: naimayat@ucla.edu
#ID: 000000000
.SILENT:
default: lab1b-client lab1b-server

lab1b-client:
	gcc -lmcrypt -o lab1b-client -g lab1b-client.c -Wall -Wextra

lab1b-server:
	gcc -lmcrypt -o lab1b-server -g lab1b-server.c -Wall -Wextra

clean:
	rm -rf lab1b-server lab1b-client *.tar.gz *.o
dist:
	tar -zcvf lab1b-000000000.tar.gz lab1b-client.c lab1b-server.c my.key Makefile README
