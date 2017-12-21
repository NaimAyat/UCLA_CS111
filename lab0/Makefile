#NAME: Naim Ayat
#EMAIL: naimayat@ucla.edu
#ID: 000000000
.SILENT:

default:
			gcc -o lab0 -g lab0.c -Wall -Wextra

check: check0 check1 check2 check3 check4

check0:
			echo "Here's a test string." > file1.txt;\
			./lab0 --input=file1.txt --output=file2.txt;\
			if [ $$? -ne 0 ]; then echo "Error: couldn't copy from input to output.";\
			fi

check1:
			./lab0 --bogus;\
			if [ $$? -ne 1 ]; then echo "Error: failed to return exit code 1 for invalid argument.";\
			fi

check2:
			./lab0 --input=file3.txt;\
			if [ $$? -ne 2 ]; then echo "Error: failed to return exit code 2 for input failure.";\
			fi

check3:
			touch file4.txt;\
			chmod 444 file4.txt;\
			./lab0 --output=file4.txt;\
			if [ $$? -ne 3 ]; then echo "Error: failed to return exit code 3 for output failure.";\
			fi

check4:
			./lab0 --segfault --catch;\
			if [ $$? -ne 4 ]; then echo "Error: failed to return exit code 4 for segmentation fault.";\
			fi

clean:
			rm -f lab0 *.o *.txt *.tar.gz

dist:
			tar -czvf lab0-000000000.tar.gz lab0.c Makefile README backtrace.png breakpoint.png
