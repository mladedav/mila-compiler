#!/bin/bash

#if ! g++ -Wall -pedantic -std=c++11 -g -c parser.cpp -o parser.o
#then
#exit 2
#fi

i=0
for file in samples/*
do
printf '%d: %s\n' $((++i)) "$file"
if ! ./parser < "$file"
then
exit 1
fi
echo
done
