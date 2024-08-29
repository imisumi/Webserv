#!/bin/bash


# for loop calling curl "localhost:8080" 1000 times
for i in {1..10}
do
  curl "localhost:8080" > /dev/null 2>&1
  echo "Request $i"
done