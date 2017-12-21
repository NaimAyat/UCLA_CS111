#NAME: Naim Ayat
#EMAIL: naimayat@ucla.edu
#ID: 000000000
build:	lab2_add.c lab2_list.c
	gcc -Wall -Wextra -pthread lab2_add.c -o lab2_add
	gcc -Wall -Wextra -pthread lab2_list.c SortedList.c -o lab2_list

tests:
	-./tests.sh

graphs:
	gnuplot lab2_add.gp
	gnuplot lab2_list.gp

dist:
	make
	make tests
	make graphs
	rm -rf lab2a-104733125.tar.gz
	tar -czf lab2a-104733125.tar.gz *.png *.gp *.csv *.c *.h Makefile README tests.sh
	
clean:
	rm -rf lab2_add lab2_list *.csv *.png lab2a-000000000.tar.gz
