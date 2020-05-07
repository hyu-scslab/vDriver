#!/bin/bash

#[Prepare]
#sysbench /root/sysbench/src/lua/oltp_read_only.lua --threads=4 --mysql-host=localhost --mysql-user=sbtest --mysql-password=sbtest --mysql-port=3306 --tables=10 --table-size=1000000 prepare
#[Run]
#Once we have our data, let’s prepare a command to run the test. We want to test Primary Key lookups therefore we will disable all other types of SELECT. We will also disable prepared statements as we want to test regular queries. We will test low concurrency, let’s say 16 threads. Our command may look like below:
#sysbench /root/sysbench/src/lua/oltp_read_only.lua --threads=16 --events=0 --time=300 --mysql-host=10.0.0.126 --mysql-user=sbtest --mysql-password=pass --mysql-port=3306 --tables=10 --table-size=1000000 --range_selects=off --db-ps-mode=disable --report-interval=1 run

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)"

cd $DIR

SYSBENCH="../sysbench/src/sysbench"
SQL="--db_driver=mysql --mysql-host=localhost --mysql-user=root --mysql-socket=/opt/sm1725-1/jaeseon/mysql/mysql-server-8.0/run/mysql.sock --mysql-port=33060 --secondary=off --create-secondary=false"
WORKLOAD="oltp_update_non_index.lua"

TIME="--time=275"
REPORT="--report-interval=1"
THREADS="--threads=48"
TABLES="--tables=48"

RAND_TYPE="zipfian"
RAND_EXP="1.2"
RECORD_NUM="--table_size=1000"

CONSOPT="--db-ps-mode=disable --range_selects=off"
RANDOPT="--rand-type=zipfian --rand-zipfian-exp=${RAND_EXP}"

TOTALOPT=$WORKLOAD" "$SQL" "$TIME" "$THREADS" "$TABLES" "$RECORD_NUM" "$RANDOPT" "$CONSOPT" "$REPORT

if [ "prepare" = "$1" ]; then
	$SYSBENCH $TOTALOPT $1

elif [ "run" = "$1" ]; then
	$SYSBENCH $TOTALOPT $1

elif [ "cleanup" = "$1" ]; then
	$SYSBENCH $TOTALOPT $1
else
	$SYSBENCH $TOTALOPT run > $1
fi

