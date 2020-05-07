#!/bin/sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)"
cd $DIR

OPER="CREATE DATABASE IF NOT EXISTS sbtest"
./bin/mysql -u root -e "${OPER}" > /dev/null 2>&1
