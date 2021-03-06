 #set terminal png
 set terminal postscript monochrome
 #set output "plot1_2.png"
 set output "plot1_2.ps"
 set title "Time taken for GC_malloc() averaged over 1000 allocations"
 set xlabel "Allocation size (B)"
 set ylabel "Pause time (us)"
 #set xrange [-2:2]
 #set yrange [-10:10]
 set zeroaxis
 #plot 'objects/plot_data' with yerrorbars

# plot_data1: my copying GC (I=65536 B=65536 A=270336 cur=65536)
# plot_data2: Boehm GC (I=65536)
 plot 'plot_data1' with lines title "My copying GC", 'plot_data2' with lines title "Boehm GC", 'plot_data3' with lines title "No GC, with free", 
 # end of test my copying GC (I=65536 B=65536 A=270336 cur=65536)
 # end of test Boehm GC (I=385024)
