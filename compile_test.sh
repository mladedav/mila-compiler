#!/bin/bash

i=0
for file in samples/*
do
printf '%d: %s\n' $((++i)) "$file"
cat "$file"
echo
if ! ./generate.sh "$file"
then
exit 1
else
read
fi
echo
done
