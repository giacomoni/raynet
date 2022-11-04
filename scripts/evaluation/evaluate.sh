#!/bin/bash
 
# Declare an array of string with type
declare -a POLICIES=($(seq 40 100 40))
 
# Iterate the string array using for loop
for val in ${POLICIES[@]}; do
   python3 batchEval.py -p $val
done