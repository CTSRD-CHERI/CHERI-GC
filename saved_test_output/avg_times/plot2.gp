 set terminal png
 set output "objects/plot.png"
 set title "Time taken for GC_malloc() averaged over 1000 allocations"
 set xlabel "Allocation size (B)"
 set ylabel "Pause time (us)"
 #set xrange [-2:2]
 #set yrange [-10:10]
 set zeroaxis
 #plot 'objects/plot_data' with yerrorbars
 plot 'objects/plot_data' with linespoints
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=65536)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=196608)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=196608)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=196608)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=196608)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=270336)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=270336)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=196608)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=196608)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=274432)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=196608)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=196608)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=196608)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=278528)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=278528)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=196608)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=196608)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=282624)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=282624)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=196608)
 # data for Boehm GC (I=65536)
 # end of test Boehm GC (I=196608)
