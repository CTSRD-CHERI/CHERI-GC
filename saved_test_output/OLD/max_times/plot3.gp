 set terminal png
 set output "plot3.png"
 set title "Maximum time taken for GC_malloc() over 1000 allocations"
 set xlabel "Allocation size (B)"
 set ylabel "Pause time (us)"
 #set xrange [-2:2]
 #set yrange [-10:10]
 set zeroaxis
 #plot 'plot_data3' with yerrorbars
 plot 'plot_data3' with linespoints
 # data for no GC
 # 1000 allocations per iteration
 # allocation size (B) pause time (us)
 # end of test no GC
