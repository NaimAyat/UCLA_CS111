#!/bin/bash

#NAME: Naim Ayat
#EMAIL: naimayat@ucla.edu
#ID: 000000000echo "" > lab2_list.csv
echo "" > lab2_add.csv

for x in 10 20 40 100 1000 10000 100000; do
  for y in 1 2 4 8 12; do
	  ./lab2_add --yield --threads=$y --iterations=$x >> lab2_add.csv
  done
done

for x in 10 20 40 100 1000 100000; do
  for y in 1 2 4 8 12; do
	 ./lab2_add --threads=$y --iterations=$x >> lab2_add.csv
  done
done

for x in 20 40 100 1000 10000; do
  for y in 1 2 4 8 12; do
	  ./lab2_add --sync=m --yield --threads=$y --iterations=$x >> lab2_add.csv
    ./lab2_add --sync=c --yield --threads=$y --iterations=$x >> lab2_add.csv
	  ./lab2_add --sync=s --yield --threads=$y --iterations=$x >> lab2_add.csv
  done
done

for x in 10 20 40 80 100 1000 10000; do
  for y in 1 2 4 8 12; do
    ./lab2_add --sync=m --threads=$y --iterations=$x >> lab2_add.csv
  	./lab2_add --sync=c --threads=$y --iterations=$x >> lab2_add.csv
  	./lab2_add --sync=s --threads=$y --iterations=$x >> lab2_add.csv
	  ./lab2_add --threads=$y --iterations=$x >> lab2_add.csv
  done
done

for x in 10 100 1000 10000 20000; do
  for y in 1; do
	   ./lab2_list --threads=$y --iterations=$x >> lab2_list.csv
  done
done

for x in 1 2 4 8 16 32; do
  for y in 2 4 8 12; do
    ./lab2_list --yield=d --threads=$y --iterations=$x >> lab2_list.csv
    ./lab2_list --yield=i --threads=$y --iterations=$x >> lab2_list.csv
    ./lab2_list --yield=il --threads=$y --iterations=$x >> lab2_list.csv
    ./lab2_list --yield=dl --threads=$y --iterations=$x >> lab2_list.csv
  done
done

for x in 1 2 4 8 16 32; do
  for y in 2 4 8 12; do
    ./lab2_list --sync=s --yield=d --threads=$y --iterations=$x >> lab2_list.csv
	  ./lab2_list --sync=s --yield=i --threads=$y --iterations=$x >> lab2_list.csv
    ./lab2_list --sync=s --yield=dl --threads=$y --iterations=$x >> lab2_list.csv
	  ./lab2_list --sync=s --yield=il --threads=$y --iterations=$x >> lab2_list.csv
    ./lab2_list --sync=m --yield=d --threads=$y --iterations=$x >> lab2_list.csv
	  ./lab2_list --sync=m --yield=i --threads=$y --iterations=$x >> lab2_list.csv
	  ./lab2_list --sync=m --yield=il --threads=$y --iterations=$x >> lab2_list.csv
    ./lab2_list --sync=m --yield=dl --threads=$y --iterations=$x >> lab2_list.csv
  done
done

for x in 1000; do
  for y in 1 2 4 8 12 16 24; do
    ./lab2_list --sync=m --threads=$y --iterations=$x >> lab2_list.csv
	  ./lab2_list --sync=s --threads=$y --iterations=$x >> lab2_list.csv
  done
done
