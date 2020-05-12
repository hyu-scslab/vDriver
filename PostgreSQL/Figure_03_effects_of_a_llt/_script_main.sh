#!/bin/bash

PORT=5439

USER=$(who am i | awk '{print $1}')
echo $USER
if  [ "$USER" == "root" ]
then
    echo "This script can't be run with root user."
    echo "Please login with normal user."
    exit
fi

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
sudo -u ${USER} mkdir -p $RESULT_DIR

# clear data and eps
rm -rf $GNUPLOT_DATA
rm -f *.eps

sudo -u ${USER} mkdir -p $GNUPLOT_DATA





# clear logfile of postgre
sudo -u ${USER} cat /dev/null > $LOGFILE




# compile sysbench
sudo -u ${USER} bash $SYSBENCH_SCRIPT"install.sh"






# clear data
sudo -u ${USER} rm -rf $POSTGRESQL_DATA





# stat-collect running time
RUNNING_TIME=100






# compile postgre
COMPILE_OPTION=""

sudo -u ${USER} bash $POSTGRESQL_INSTALL_SCRIPT"install.sh" --compile_option="$COMPILE_OPTION"





# init data
sudo -u ${USER} bash $POSTGRESQL_SERVER_SCRIPT"init_data.sh"

# copy conf file to postgre dir
sudo -u ${USER} cp $CONFIG_DIR"postgresql.conf" $POSTGRESQL_DATA

# run postgresql server
sudo -u ${USER} bash $POSTGRESQL_SERVER_SCRIPT"run_server.sh" --logfile="$LOGFILE"

sleep 2

# init user, database, privilege
sudo -u ${USER} bash ${POSTGRESQL_CLIENT_SCRIPT}"init_db.sh" --port="$PORT"

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
sudo -u ${USER} $SYSBENCH $SYSBENCH_OPT $SYSBENCH_WORKLOAD "cleanup"
sudo -u ${USER} $SYSBENCH $SYSBENCH_OPT $SYSBENCH_WORKLOAD "prepare"
cd $DIR



sleep 2


# make long transaction
QUERY="\c sbtest \\\ BEGIN; SELECT txid_current(); SELECT * FROM sbtest1 WHERE id = 3; \
       SELECT pg_sleep(60); \
       COMMIT;"
( sleep 020 ; \
  sudo -u ${USER} date +%s.%N > "${RESULT_DIR}longx_begin.data"; \
  sudo -u ${USER} bash ${POSTGRESQL_CLIENT_SCRIPT}"run_query.sh" --output="${RESULT_DIR}longx_xid.data" --query="$QUERY" --port="$PORT"; \
  sudo -u ${USER} date +%s.%N > "${RESULT_DIR}longx_end.data"; ) &

# perf
for ((i=3;i<"$RUNNING_TIME";i+=3)); do
	(sleep $(expr $i - 1) ; timeout 2 perf record -g -a --output="$RESULT_DIR"perf_$i.data"") &
done

# vmstat
for ((i=3;i<"$RUNNING_TIME";i+=3)); do
	(sleep $i ; sudo -u ${USER} vmstat -t 1 2 > "$RESULT_DIR"vmstat_$i.data"") &
done

# recent xid
for ((i=1;i<"$RUNNING_TIME"*3;i+=1)); do
	(sleep "$((1000000000 *   $i/3  ))e-9" ; \
	 (date +%s.%N ; sudo -u ${USER} bash ${POSTGRESQL_CLIENT_SCRIPT}"run_query.sh" --query="SELECT txid_current();" --port="$PORT") \
	 > "${RESULT_DIR}recent_xid_${i}.data") &
done


# run sysbench
( sudo -u ${USER} $SYSBENCH $SYSBENCH_OPT $SYSBENCH_WORKLOAD run ) > "${RESULT_DIR}sysbench.data"

wait

sleep 2

# shutdown postgresql server
sudo -u ${USER} bash $POSTGRESQL_SERVER_SCRIPT"shutdown_server.sh"

sleep 5





# copy logfile of postgre to log dir
sudo -u ${USER} cp $LOGFILE $RESULT_DIR


wait

# copy all config files to log directory
sudo -u ${USER} cp $CONFIG_DIR"postgresql.conf" $0 $RESULT_DIR
sudo -u ${USER} cp $DIR"refine.py" $DIR"plot.script" $RESULT_DIR

# copy data for gnuplot.
find ${RESULT_DIR} -type f -name "*.data" -exec cp -f {} ${GNUPLOT_DATA} \;
find ${GNUPLOT_DATA} -type f -name "*.data" -exec chown ${USER}:${USER} {} \;

# perf data to text file
for entry in ${GNUPLOT_DATA}perf_*.data
do
	(perf report -s sym --children -f -i $entry --stdio > $entry".txt" 2> /dev/null ; chown ${USER}:${USER} $entry".txt") &
done
wait

# refine
sudo -u ${USER} python3 refine.py

# plot
sudo -u ${USER} gnuplot plot.script

sudo -u ${USER} cp *.eps $RESULT_DIR






