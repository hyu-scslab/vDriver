#!/bin/bash

#[Prepare]
#sysbench /root/sysbench/src/lua/oltp_read_only.lua --threads=4 --mysql-host=localhost --mysql-user=sbtest --mysql-password=sbtest --mysql-port=3306 --tables=10 --table-size=1000000 prepare
#[Run]
#Once we have our data, let’s prepare a command to run the test. We want to test Primary Key lookups therefore we will disable all other types of SELECT. We will also disable prepared statements as we want to test regular queries. We will test low concurrency, let’s say 16 threads. Our command may look like below:
#sysbench /root/sysbench/src/lua/oltp_read_only.lua --threads=16 --events=0 --time=300 --mysql-host=10.0.0.126 --mysql-user=sbtest --mysql-password=pass --mysql-port=3306 --tables=10 --table-size=1000000 --range_selects=off --db-ps-mode=disable --report-interval=1 run

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)"

cd $DIR

SYSBENCH="../sysbench/src/sysbench"

SQL="--db_driver=mysql --mysql-host=localhost --mysql-user=root --mysql-port=33060 --secondary=off --create_secondary=false"
WORKLOAD="oltp_update_non_index_og.lua"
TIME="--time=100"
REPORT="--report-interval=2"
THREADS="--threads=48 --db-ps-mode=disable --range_selects=off"
TABLES="--tables=1"
# record number per one table
RECORD_NUM="--table_size=100000"

PATH="--mysql-socket="
SOCKET="path_to_socket"
PATH+=$SOCKET

TOTALOPT=$WORKLOAD" "$SQL" "$PATH" "$TIME" "$THREADS" "$TABLES" "$RECORD_NUM" "$REPORT

if [ "prepare" = "$1" ]; then
	$SYSBENCH $TOTALOPT $1

elif [ "run" = "$1" ]; then
	$SYSBENCH $TOTALOPT $1

elif [ "cleanup" = "$1" ]; then
	$SYSBENCH $TOTALOPT $1
else
	$SYSBENCH $TOTALOPT run > $1
fi
