 #set terminal png
 set terminal postscript monochrome
 #set output "plot1_2.png"
 set output "plot1_2.ps"
 set title "Time taken by GC_malloc() averaged over 1000 consecutive allocations and 10 independent runs for every 1kB"
 set xlabel "Allocation size (kB)"
 set ylabel "Pause time (ms)"
 #set xrange [-2:2]
 #set yrange [-10:10]
 set zeroaxis
 #plot 'objects/plot_data' with yerrorbars

# `unique' averages the y values of all same x values
# Boehm: (initial heap size 65,536 B, final heap size: 196,608 B (for 5kB and 6kB, 270,336 B) (for 9kB, 274,432 B) (for 13kB and 14kB, 278,528 B) (for 17kB and 18kB, 282,624 B))
 plot 'plot_data1' using ($1/1000.):($2/1000.) smooth unique with lines title "My copying GC (semispace size 815,104 B)", \
      'plot_data2' using ($1/1000.):($2/1000.) smooth unique with lines title "Boehm GC (initial heap 65,536 B, final heap 200-300kB)", \
      'plot_data3' using ($1/1000.):($2/1000.) smooth unique with lines title "No GC (with free)", 
 
