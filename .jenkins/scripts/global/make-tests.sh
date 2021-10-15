#!/bin/bash

for dir in tests/*/
do
  START=$SECONDS
  make tests tests/$dir
  CURR=$SECONDS
  DUR=$(($CURR - $START))
  echo "$dir: $DUR" >> tests.log
done

echo "===================== Test results =========================="
cat tests.log
