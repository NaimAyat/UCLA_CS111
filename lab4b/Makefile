#NAME:	Naim Ayat
#EMAIL:	naimayat@ucla.edu
#ID: 000000000

default:
	gcc -Wall -Wextra -lmraa -lm -pthread -o lab4b lab4b.c

.SILENT:
check:  default
	./lab4b --period=1 --bogus; \
	if [[ $$? -ne 1 ]]; then \
	echo "ERROR: failed to detect incorrect long option.";
	fi
	./lab4b --period=1 --scale=bogus; \
	if [[ $$? -ne 1 ]]; then \
	echo "ERROR: failed to detect incorrect period argument.";
	fi

clean:
	rm -f lab4b lab4b-000000000.tar.gz

dist:
	tar -czf lab4b-000000000.tar.gz lab4b.c README Makefile
