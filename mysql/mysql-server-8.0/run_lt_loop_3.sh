#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)"
cd $DIR

OPER="START TRANSACTION;"
OPER+="CALL LT_3();"
OPER+="COMMIT";

run/bin/mysql -u root sbtest -e "${OPER}" > /dev/null 2>&1 
