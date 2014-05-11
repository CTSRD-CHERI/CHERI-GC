for f in run_output*
do
  cat $f | grep "\[plotdata\]" | cut -d" " -f2- > `echo plot_data\`echo $f | sed s/run_output//\``
done
