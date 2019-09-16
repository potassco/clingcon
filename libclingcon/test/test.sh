#!/bin/bash

if [ -z "$1" ]
then
    echo "1"
    executable=../../build/bin/testclingcon
else
    executable=${1}
    echo "2"
fi
${executable}
num=$?
echo $num
for i in $(seq 0 $((${num}-1)));
do
    echo $i
    ${executable} $i;
    if (($? > 0)); then
       RET=1
       echo "Error in test $i"
       exit 1;
    fi
done
