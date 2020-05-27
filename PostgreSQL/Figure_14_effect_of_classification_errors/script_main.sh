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
rm -f *.eps

mkdir -p $GNUPLOT_DATA

# clear logfile of postgre
cat /dev/null > $LOGFILE

# compile sysbench
bash $SYSBENCH_SCRIPT"install.sh"

######################################################### iter begin
DISTRIBUTION_LIST="UNIFORM SKEWED"
CLASSIFICATION_LIST="NORMAL VULNERABLE"

for distribution in $DISTRIBUTION_LIST
do

for classification in $CLASSIFICATION_LIST
do

# clear data
rm -rf $POSTGRESQL_DATA

# stat-collect running time
RUNNING_TIME=40

# compile postgre
if [ "$classification" == "NORMAL" ]
then
COMPILE_OPTION="-DHYU_LLT -DHYU_LLT_STAT -DHYU_COMMON_STAT -DINTERVAL_UPDATE_DEADZONE=0.1 -DVBUFFER_NUM_PAGE=100000"
elif [ "$classification" == "VULNERABLE" ]
then
COMPILE_OPTION="-DHYU_LLT -DHYU_LLT_STAT -DHYU_COMMON_STAT -DINTERVAL_UPDATE_DEADZONE=0.1 -DVBUFFER_NUM_PAGE=100000 -DCLASSIFICATION_THRESHOLD_LLT=40000"
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
SYSBENCH_OPT+=" --time=35"
SYSBENCH_OPT+=" --threads=48"
SYSBENCH_OPT+=" --tables=48" 
SYSBENCH_OPT+=" --table_size=10000" 
SYSBENCH_OPT+=" --rand-type=zipfian"
if [ "$distribution" == "UNIFORM" ]
then
SYSBENCH_OPT+=" --rand-zipfian-exp=0.0"
elif [ "$distribution" == "SKEWED" ]
then
SYSBENCH_OPT+=" --rand-zipfian-exp=1.2"
fi
SYSBENCH_WORKLOAD=$SYSBENCH_LUA"oltp_update_non_index.lua"
cd $SYSBENCH_LUA
$SYSBENCH $SYSBENCH_OPT $SYSBENCH_WORKLOAD "cleanup"
$SYSBENCH $SYSBENCH_OPT $SYSBENCH_WORKLOAD "prepare"
cd $DIR

sleep 2

# make long transaction
QUERY="\c sbtest \\\ BEGIN; SELECT txid_current(); SELECT * FROM sbtest1 WHERE id = 10; \
       SELECT pg_sleep(30); \
       COMMIT;"
( sleep 1 ; \
  date +%s.%N > "${RESULT_DIR}longx_01_begin_${distribution}_${classification}.data"; \
  bash ${POSTGRESQL_CLIENT_SCRIPT}"run_query.sh" --query="$QUERY" --port="$PORT"; \
  date +%s.%N > "${RESULT_DIR}longx_01_end_${distribution}_${classification}.data"; ) &

# get stat
(sleep 32 ; \
     (date +%s.%N ; bash ${POSTGRESQL_CLIENT_SCRIPT}"run_query.sh" --query="SELECT llt_get_cuttime();" --port="$PORT") \
     > "${RESULT_DIR}llt_get_cuttime_${distribution}_${classification}.data") &

# run sysbench
( ( sleep 000; cd $SYSBENCH_LUA; $SYSBENCH $SYSBENCH_OPT $SYSBENCH_WORKLOAD run ; cd $DIR) > "${RESULT_DIR}sysbench_${distribution}_${classification}.data") &

wait

sleep 2

# shutdown postgresql server
bash $POSTGRESQL_SERVER_SCRIPT"shutdown_server.sh"

sleep 5

done
######################################################### CLASSIFICATION_LIST iter end
done
######################################################### DISTRIBUTION_LIST iter end

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

