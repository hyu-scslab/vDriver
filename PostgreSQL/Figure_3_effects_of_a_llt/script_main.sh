#!/bin/bash

PORT=5439


DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"/

cd $DIR

# path of each script
POSTGRESQL_DATA=$DIR"../PostgreSQL/data/"
POSTGRESQL_SERVER_SCRIPT=$DIR"../PostgreSQL/postgres_script/script_server/"
POSTGRESQL_CLIENT_SCRIPT=$DIR"../PostgreSQL/postgres_script/script_client/"
POSTGRESQL_INSTALL_SCRIPT=$DIR"../PostgreSQL/postgres_script/script_install/"

SYSBENCH_BASE=$DIR"../sysbench/"
SYSBENCH=$SYSBENCH_BASE"sysbench/src/sysbench"
SYSBENCH_LUA=$SYSBENCH_BASE"sysbench/src/lua/"
SYSBENCH_SCRIPT=$SYSBENCH_BASE"sysbench_script/"

CONFIG_DIR=$DIR"config/"

# logfile path
LOGFILE=$DIR"logfile"

# data dir for gnuplot
GNUPLOT_DATA=$DIR"gnuplot_data/"

# date and time
NOW="$(date +%Y%m%d)_$(date +%H%M%S)"
echo $NOW
echo "bench script"

# make a directory for log and result
# NOW+="add comment"
RESULT_DIR=$DIR"result/$NOW/"
mkdir -p $RESULT_DIR

# clear data and eps
sudo rm -rf $GNUPLOT_DATA
sudo rm -f *.eps

mkdir -p $GNUPLOT_DATA





# clear logfile of postgre
cat /dev/null > $LOGFILE




# compile sysbench
#bash $SYSBENCH_SCRIPT"install.sh"






# clear data
rm -rf $POSTGRESQL_DATA





# stat-collect running time
RUNNING_TIME=100






# compile postgre
#COMPILE_OPTION="-DHYU_COMMON_STAT"

#bash $POSTGRESQL_INSTALL_SCRIPT"install.sh" --compile_option="$COMPILE_OPTION"





# init data
bash $POSTGRESQL_SERVER_SCRIPT"init_data.sh"

# copy conf file to postgre dir
cp $CONFIG_DIR"postgresql.conf" $POSTGRESQL_DATA

# run postgresql server
bash $POSTGRESQL_SERVER_SCRIPT"run_server.sh" --logfile="$LOGFILE"

sleep 2

# init user, database, privilege
bash ${POSTGRESQL_CLIENT_SCRIPT}"init_db.sh" --port="$PORT"

sleep 2

# prepare sysbench
SYSBENCH_OPT=""
SYSBENCH_OPT+=" --db-driver=pgsql --pgsql-user=sbtest --pgsql-db=sbtest --report-interval=1 --secondary=off --pgsql-port=${PORT}"
SYSBENCH_OPT+=" --time=$(expr $RUNNING_TIME + 10)"
SYSBENCH_OPT+=" --threads=48"
SYSBENCH_OPT+=" --tables=1" 
SYSBENCH_OPT+=" --table_size=100000" 
SYSBENCH_OPT+=" --rand-type=zipfian --rand-zipfian-exp=0.8" 
SYSBENCH_WORKLOAD=$SYSBENCH_LUA"oltp_update_non_index.lua"
cd $SYSBENCH_LUA
$SYSBENCH $SYSBENCH_OPT $SYSBENCH_WORKLOAD "cleanup"
$SYSBENCH $SYSBENCH_OPT $SYSBENCH_WORKLOAD "prepare"
cd $DIR



sleep 2


# make long transaction
QUERY="\c sbtest \\\ BEGIN; SELECT txid_current(); SELECT * FROM sbtest1 WHERE id = 3; \
       SELECT pg_sleep(60); \
       COMMIT;"
( sleep 020 ; \
  date +%s.%N > "${RESULT_DIR}longx_begin.data"; \
  bash ${POSTGRESQL_CLIENT_SCRIPT}"run_query.sh" --output="${RESULT_DIR}longx_xid.data" --query="$QUERY" --port="$PORT"; \
  date +%s.%N > "${RESULT_DIR}longx_end.data"; ) &

# perf
for ((i=3;i<"$RUNNING_TIME";i+=3)); do
	(sleep $(expr $i - 1) ; sudo timeout 2 perf record -g -a --output="$RESULT_DIR"perf_$i.data"") &
done

# vmstat
for ((i=3;i<"$RUNNING_TIME";i+=3)); do
	(sleep $i ; vmstat -t 1 2 > "$RESULT_DIR"vmstat_$i.data"") &
done

# recent xid
for ((i=1;i<"$RUNNING_TIME"*3;i+=1)); do
	(sleep "$((1000000000 *   $i/3  ))e-9" ; \
	 (date +%s.%N ; bash ${POSTGRESQL_CLIENT_SCRIPT}"run_query.sh" --query="SELECT txid_current();" --port="$PORT") \
	 > "${RESULT_DIR}recent_xid_${i}.data") &
done


# run sysbench
( $SYSBENCH $SYSBENCH_OPT $SYSBENCH_WORKLOAD run ) > "${RESULT_DIR}sysbench.data"

wait

sleep 2

# shutdown postgresql server
bash $POSTGRESQL_SERVER_SCRIPT"shutdown_server.sh"

sleep 5





# copy logfile of postgre to log dir
cp $LOGFILE $RESULT_DIR

# edit logfile

(sed '/HYU_LLT/!d' logfile > $RESULT_DIR"verification.data") &

wait

# copy all config files to log directory
cp $CONFIG_DIR"postgresql.conf" $0 $RESULT_DIR
cp $DIR"refine.py" $DIR"plot.script" $RESULT_DIR

# copy data for gnuplot.
sudo find ${RESULT_DIR} -type f -name "*.data" -exec cp -f {} ${GNUPLOT_DATA} \;

# refine
python3 refine.py

# plot
gnuplot plot.script

cp *.eps $RESULT_DIR






