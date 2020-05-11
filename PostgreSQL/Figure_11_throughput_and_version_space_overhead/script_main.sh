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
rm -rf $GNUPLOT_DATA
rm -f *.eps

mkdir -p $GNUPLOT_DATA





# clear logfile of postgre
cat /dev/null > $LOGFILE




# compile sysbench
bash $SYSBENCH_SCRIPT"install.sh"

######################################################### iter begin
ENGINE_LIST="VANILLA VDRIVER"
#ENGINE_LIST="VANILLA"
#ENGINE_LIST="VDRIVER"
for engine in $ENGINE_LIST
do





# clear data
rm -rf $POSTGRESQL_DATA





# stat-collect running time
RUNNING_TIME=620






# compile postgre
if [ "$engine" == "VANILLA" ]
then
COMPILE_OPTION="-DHYU_COMMON_STAT"
fi
if [ "$engine" == "VDRIVER" ]
then
COMPILE_OPTION="-DHYU_LLT -DHYU_LLT_STAT -DHYU_COMMON_STAT -DINTERVAL_UPDATE_DEADZONE=0.1"
fi

bash $POSTGRESQL_INSTALL_SCRIPT"install.sh" --compile_option="$COMPILE_OPTION"
#bash $POSTGRESQL_INSTALL_SCRIPT"install.sh" --compile_option="$COMPILE_OPTION" --no_configure





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
SYSBENCH_OPT+=" --time=275"
SYSBENCH_OPT+=" --threads=48"
SYSBENCH_OPT+=" --tables=48" 
SYSBENCH_OPT+=" --table_size=1000" 
SYSBENCH_WORKLOAD=$SYSBENCH_LUA"oltp_update_non_index.lua"
cd $SYSBENCH_LUA
$SYSBENCH $SYSBENCH_OPT $SYSBENCH_WORKLOAD "cleanup"
$SYSBENCH $SYSBENCH_OPT $SYSBENCH_WORKLOAD "prepare"
cd $DIR



sleep 2


# make long transaction
QUERY="\c sbtest \\\ BEGIN; SELECT txid_current(); SELECT * FROM sbtest1 WHERE id = 10; \
       SELECT pg_sleep(200); \
       COMMIT;"
( sleep 050 ; \
  date +%s.%N > "${RESULT_DIR}longx_01_begin_${engine}.data"; \
  bash ${POSTGRESQL_CLIENT_SCRIPT}"run_query.sh" --query="$QUERY" --port="$PORT"; \
  date +%s.%N > "${RESULT_DIR}longx_01_end_${engine}.data"; ) &
( sleep 350 ; \
  date +%s.%N > "${RESULT_DIR}longx_02_begin_${engine}.data"; \
  bash ${POSTGRESQL_CLIENT_SCRIPT}"run_query.sh" --query="$QUERY" --port="$PORT"; \
  date +%s.%N > "${RESULT_DIR}longx_02_end_${engine}.data"; ) &


# get stat
if [ "$engine" == "VANILLA" ]
then
for ((i=0;i<"$RUNNING_TIME";i+=1)); do
    QUERY="\c sbtest \\\ BEGIN; \
           SELECT * FROM sbtest3 WHERE id = 1; \
           SELECT get_stat(); \
           COMMIT;"
    ( sleep ${i} ; bash ${POSTGRESQL_CLIENT_SCRIPT}"run_query.sh" --query="$QUERY" --port="$PORT" \
      > "${RESULT_DIR}stat_${engine}_${i}.data"; ) &
done
fi


if [ "$engine" == "VDRIVER" ]
then

QUERY="\c sbtest \\\ BEGIN; SELECT * FROM sbtest3 WHERE id = 11; "
for ((i=0;i<"$RUNNING_TIME";i+=1)); do
           QUERY+="SELECT * FROM sbtest3 WHERE id = 1; \
           SELECT get_stat(); \
           SELECT pg_sleep(1); \
           "
done
QUERY+="COMMIT;"
( sleep 0 ; bash ${POSTGRESQL_CLIENT_SCRIPT}"run_query.sh" --query="$QUERY" --port="$PORT" \
      > "${RESULT_DIR}stat_${engine}.data"; ) &

fi






# recent xid
for ((i=1;i<"$RUNNING_TIME"*1;i+=1)); do
	(sleep "$((1000000000 *   $i/1  ))e-9" ; \
	 (date +%s.%N ; bash ${POSTGRESQL_CLIENT_SCRIPT}"run_query.sh" --query="SELECT txid_current();" --port="$PORT") \
	 > "${RESULT_DIR}recent_xid_${engine}_${i}.data") &
done

# data directory & redo log size
for ((i=1;i<"$RUNNING_TIME"*1;i+=1)); do
	(sleep "$((1000000000 *   $i/1  ))e-9" ; \
	 (date +%s.%N ; du -sb "${POSTGRESQL_DATA}"; du -sb "${POSTGRESQL_DATA}pg_wal";) \
	 > "${RESULT_DIR}size_${engine}_${i}.data") &
done

# run sysbench
PHASE_01_SYSBENCH_OPT="${SYSBENCH_OPT} --rand-type=zipfian --rand-zipfian-exp=0.0" 
PHASE_02_SYSBENCH_OPT="${SYSBENCH_OPT} --rand-type=zipfian --rand-zipfian-exp=1.2" 
( ( sleep 005; cd $SYSBENCH_LUA; $SYSBENCH $PHASE_01_SYSBENCH_OPT $SYSBENCH_WORKLOAD run ; cd $DIR) > "${RESULT_DIR}sysbench_phase_01_${engine}.data") &
( ( sleep 305; cd $SYSBENCH_LUA; $SYSBENCH $PHASE_02_SYSBENCH_OPT $SYSBENCH_WORKLOAD run ; cd $DIR) > "${RESULT_DIR}sysbench_phase_02_${engine}.data") &


wait

sleep 2

# shutdown postgresql server
bash $POSTGRESQL_SERVER_SCRIPT"shutdown_server.sh"

sleep 5


done
######################################################### WORKLOAD_LIST iter end




# copy logfile of postgre to log dir
cp $LOGFILE $RESULT_DIR

# edit logfile

(sed '/HYU_LLT/!d' logfile > $RESULT_DIR"verification.data") &

wait

# copy all config files to log directory
cp $CONFIG_DIR"postgresql.conf" $0 $RESULT_DIR
cp $DIR"refine.py" $DIR"plot.script" $RESULT_DIR

# copy data for gnuplot.
find ${RESULT_DIR} -type f -name "*.data" -exec cp -f {} ${GNUPLOT_DATA} \;

# refine
python3 refine.py

# plot
gnuplot plot.script

cp *.eps $RESULT_DIR






