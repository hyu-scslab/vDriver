#!/bin/sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)"
cd $DIR

rm -rf data/

# Initialize server
./bin/mysqld --initialize-insecure
