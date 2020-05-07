#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)"
cd $DIR

OPER="START TRANSACTION;"
OPER+="SELECT id from sbtest1 where id=1; SELECT sleep(60); SELECT id from sbtest1 where id=1;"
OPER+="COMMIT";

run/bin/mysql -u root sbtest -e "${OPER}" > /dev/null 2>&1 
