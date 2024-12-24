#!/bin/bash
clear
make
./main 13
# strace -ff -o /tmp/trace ./main 13
make clean
