#!/bin/bash

i=0
for file in samples/*
do
printf '%d: %s\n' $((++i)) "$file"
if ! ./lexan < "$file" > tmp_output1 || ! ./lexan "$file" > tmp_output2 || ! diff tmp_output1 tmp_output2
then
rm tmp_output{1,2}
exit 1
fi
done
rm tmp_output{1,2}
