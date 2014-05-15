# this script is pushed to and run on cloud2 by Makefile.testall
prog=PROG
i=32768
while [ $i -le 64000 ]
do
  j=1
  while [ $j -le 3 ]
  do
    $prog "((fn f . (fn g. (f (fn a . (g g) a))) (fn g. (f (fn f . (g g) f)))) (fn f . fn n . if n then n + f (n-1) else 1)) $i" $i
    echo testall.sh: i=$i j=$j
    j=`expr $j + 1`
  done
  i=`expr $i \* 2`
done
