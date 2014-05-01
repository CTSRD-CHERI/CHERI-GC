 set terminal png
 set output "plot1.png"
 set title "Time taken for GC_malloc() averaged over 1000 allocations of each size and 10 runs for each size"
 set xlabel "Allocation size (B)"
 set ylabel "Pause time (us)"
 #set xrange [-2:2]
 #set yrange [-10:10]
 set zeroaxis
 #plot 'plot_data1' with yerrorbars
 plot 'plot_data1' smooth unique with linespoints
