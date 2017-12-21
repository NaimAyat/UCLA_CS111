#! /usr/bin/gnuplot

set terminal png
set datafile separator ","

set title "throughput vs. number of threads for mutex and spin-lock synchronized list operations"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "throughput (op/s)"
set logscale y 10
set output 'lab2b_1.png'
set key left top

plot \
     "< grep add-m lab2_add.csv" using ($2):(1000000000/($6)) \
	title 'mutex add, 10000 iters' with linespoints lc rgb 'red', \
     "< grep add-s lab2_add.csv" using ($2):(1000000000/($6)) \
	title 'spin-lock add, 10000 iters' with linespoints lc rgb 'green', \
     "< grep list-none-m lab2_list.csv" using ($2):(1000000000/($6)) \
	title 'mutex list, 1000 iters' with linespoints lc rgb 'blue', \
     "< grep list-none-s lab2_list.csv" using ($2):(1000000000/($6)) \
	title 'spin-lock list, 1000 iters' with linespoints lc rgb 'orange'

set title "mean time per mutex wait and mean time per operation for mutex-synchronized list operations"
set xlabel "Threads"
set logscale x 2
set ylabel "mean time for wait/op"
set logscale y 10
set output 'lab2b_2.png'

plot \
     "< grep list-none-m lab_2b_list.csv | grep 1000,1," using ($2):($7) \
	title 'mean time per operation' with linespoints lc rgb 'red', \
     "< grep list-none-m lab_2b_list.csv | grep 1000,1," using ($2):($8) \
	title 'mean time per lock' with linespoints lc rgb 'green'

set title "successful iterations vs. threads for each synchronization method"
set xlabel "Threads"
set logscale x 2
set ylabel "Iterations"
set logscale y 10
set output 'lab2b_3.png'

plot \
     "< grep list-id-m lab_2b_list.csv" using ($2):($3) \
	title 'mutex' with points lc rgb 'red', \
     "< grep list-id-s lab_2b_list.csv" using ($2):($3) \
	title 'spin-lock' with points lc rgb 'green', \
     "< grep list-id-none lab_2b_list.csv" using ($2):($3) \
	title 'none' with points lc rgb 'blue'

set title "throughput vs. number of threads for mutex synchronized partitioned lists"
set xlabel "Threads"
set logscale x 2
set ylabel "throughput (op/s)"
set output 'lab2b_4.png'
set key right top

plot \
     "< grep list-none-m lab_2b_list.csv | grep 1000,1," using ($2):(1000000000/($6)) \
	title 'list = 1' with linespoints lc rgb 'red', \
     "< grep list-none-m lab_2b_list.csv | grep 1000,4," using ($2):(1000000000/($6)) \
	title 'lists = 4' with linespoints lc rgb 'green', \
     "< grep list-none-m lab_2b_list.csv | grep 1000,8," using ($2):(1000000000/($6)) \
	title 'lists = 8' with linespoints lc rgb 'blue', \
     "< grep list-none-m lab_2b_list.csv | grep 1000,16," using ($2):(1000000000/$6) \
	title 'lists = 16' with linespoints lc rgb 'orange'

set title "throughput vs. number of threads for spin-lock-synchronized partitioned lists"
set xlabel "Threads"
set logscale x 2
set ylabel "throughput (op/s)"
set output 'lab2b_5.png'
set key right top

plot \
     "< grep list-none-s lab_2b_list.csv | grep 1000,1," using ($2):(1000000000/($6)) \
	title 'list = 1' with linespoints lc rgb 'red', \
     "< grep list-none-s lab_2b_list.csv | grep 1000,4," using ($2):(1000000000/($6)) \
	title 'lists = 4' with linespoints lc rgb 'green', \
     "< grep list-none-s lab_2b_list.csv | grep 1000,8," using ($2):(1000000000/($6)) \
	title 'lists = 8' with linespoints lc rgb 'blue', \
     "< grep list-none-s lab_2b_list.csv | grep 1000,16," using ($2):(1000000000/($6)) \
	title 'lists = 16' with linespoints lc rgb 'orange'