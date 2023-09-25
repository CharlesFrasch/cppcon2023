#! /usr/bin/bash

for bench in fifo2 fifo3 fifo4 fifo4a fifo4b fifo5 fifo5a rigtorp boost_lockfree mutex;
do
./build/release/$bench 1 2
done
