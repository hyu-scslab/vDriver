#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 &&pwd)"
cd $DIR

# make clean;

make -j64;
make -j32 install;
