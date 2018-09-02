#! /bin/bash

if [ $# -gt 1 ]
then
name=$2
else
name=${1##*/}
name=${name%%.p}
fi
#echo $name
./parser < "${1}" &&
llc binary/"$name" -filetype=obj -o binary/"$name".o &&
gcc binary/"$name".o binary/inc.o -o binary/a.out &&
binary/a.out 

