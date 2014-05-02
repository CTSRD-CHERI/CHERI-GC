 set terminal png
 set output "objects/plot.png"
 set title "Time taken by GC_malloc() for 1000 allocations"
 set xlabel "Allocation size (B)"
 set ylabel "Pause time (us)"
 #set xrange [-2:2]
 #set yrange [-10:10]
 set zeroaxis
 #plot 'objects/plot_data' with yerrorbars
 plot 'objects/plot_data' with linespoints
