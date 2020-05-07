#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)"
cd $DIR
OPER="SELECT sleep(10);"
OPER+="START TRANSACTION;"
OPER+="SELECT * FROM sbtest1;"
OPER+="SELECT sleep(60);"
OPER+="SELECT * FROM sbtest1; COMMIT;"


run/bin/mysql -u root sbtest -e "${OPER}" > /dev/null 2>&1
#run/bin/mysql -u root < long.sql > /dev/null 2>&1 
