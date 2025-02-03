#!/bin/bash

clear
make
./parent
# strace -ff -o /tmp/trace ./parent
make clean
