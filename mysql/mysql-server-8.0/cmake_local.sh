#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)"
cd $DIR

rm -f CMakeCache.txt
rm -f VERSION.dep
rm -rf CMakeFiles/
rm -rf run/

export MY_DB_PATH=$DIR

cmake  \
	-DCMAKE_INSTALL_PREFIX=$MY_DB_PATH/run \
	-DSYSCONFDIR=$MY_DB_PATH \
	-DMYSQL_TCP_PORT=3306 \
	-DDEFAULT_CHARSET=utf8 \
	-DWITH_EXTRA_CHARSETS=all \
	-DDEFAULT_COLLATION=utf8_general_ci \
	-DMYSQL_UNIX_ADDR=$MY_DB_PATH/run/mysql.sock \
	-DMYSQL_DATADIR=$MY_DB_PATH/data \
	-DDOWNLOAD_BOOST=1 \
	-DWITH_BOOST=/usr/local/boost_1_69_0/ \
	-DFORCE_INSOURCE_BUILD=1 \
	-DCMAKE_BUILD_TYPE=Release \
	-DWITH_DEBUG=0 \
