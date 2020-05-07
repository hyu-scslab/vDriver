#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)"
cd $DIR

OPER="START TRANSACTION READ ONLY;"
OPER+="CALL LT_VD("${1}");"
OPER+="COMMIT";
run/bin/mysql -u root sbtest -e "${OPER}" > /dev/null 2>&1
