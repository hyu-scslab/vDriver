#!/bin/sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)"
cd $DIR

DATADIR=$DIR/data/

rm -f logfile.err
rm -f data/vseg.*
rm -f data/undo_001
rm -f data/undo_002
rm -f data/ib_logfile0
rm -f data/ib_logfile1

./run/bin/mysqld --log_error=$DIR/logfile.err --datadir=$DATADIR --user=root
