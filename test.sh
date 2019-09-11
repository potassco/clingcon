#!/bin/bash

./build/bin/testclingcon
num=$?
for i in $(seq 0 $((${num}-1))); do echo $i && ./build/bin/testclingcon $i; done
