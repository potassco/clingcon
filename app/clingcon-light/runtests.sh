#!/bin/bash


cd build/debug/
./test_liborder
NUM=`expr $? - 1`
for i in `seq 0 $NUM`; do
   ./test_liborder $i
   if (($? > 0)); then
      echo "Error in test $i"
   fi
   echo -n .
done
