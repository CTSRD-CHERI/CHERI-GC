# this script is pushed to and run on cloud2 by Makefile.testall
prog=/mnt/mbv21zfs/testall-boehm
$prog 0
i=1
while [ $i -le 20 ]
do
  j=1
  while [ $j -le 10 ]
  do
    $prog $i data
    echo testall.sh: i=$i j=$j
    j=`expr $j + 1`
  done
  i=`expr $i + 1`
done
