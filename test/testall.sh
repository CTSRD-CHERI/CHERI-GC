# this script is pushed to and run on cloud2 by Makefile.testall
prog=/mnt/mbv21zfs/testall
$prog 0
i=1
while [ $i -le 20 ]
do
  j=1
  while [ $j -le 3 ]
  do
    #arg="((fn f . (fn g. (f (fn a . (g g) a))) (fn g. (f (fn f . (g g) f)))) (fn f . fn n . if n then n + f (n-1) else 1)) $i" $i
    $prog $i data
    echo testall.sh: i=$i j=$j
    j=`expr $j + 1`
  done
  #i=`expr $i * 2`
  i=`expr $i + 1`
done
