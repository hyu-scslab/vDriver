#!/bin/sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)"
cd $DIR

DATADIR=$DIR/data/
rm -rf data/

# Initialize server
./bin/mysqld --defaults-file=$DIR/my.cnf --initialize-insecure --datadir=$DATADIR
