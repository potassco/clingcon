#!/bin/bash


cd build/debug/
./testclingcon
NUM=`expr $? - 1`
RET=0
for i in `seq 0 $NUM`; do
   ./testclingcon $i
   if (($? > 0)); then
      RET=1
      echo "Error in test $i"
   fi
   echo -n .
done
exit $RET
