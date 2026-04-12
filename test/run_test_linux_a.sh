#!/bin/bash

TEST_DIR="./build"
REPORT_FILE="$TEST_DIR/report.txt"
ALL_FILE="$TEST_DIR/all.txt"
FAIL_FILE="$TEST_DIR/fail.txt"

run_test() {
    local cmd="$1"
    local arg="${2:-}"

    if "$cmd" "$arg" &> "$REPORT_FILE"; then
        echo "Passed: $cmd $arg"
    else
        echo "Failed: $cmd $arg"
        cat "$REPORT_FILE" >> "$FAIL_FILE"
    fi

    cat "$REPORT_FILE" >> "$ALL_FILE"
}


rm -f "$ALL_FILE" "$FAIL_FILE" "$REPORT_FILE"

# This is not supported by freetds driver
#	'odbc:DSN=MSSQL;UID=root;PWD=rootroot;@engine=mssql' \
#	'odbc:DSN=MSSQL;UID=root;PWD=rootroot;@engine=mssql;@utf=wide' \

for STR in \
	'sqlite3:db=test.db' \
	'mysql:database=test1;user=test1;password=test1' \
	# 'postgresql:dbname=test' \
	# 'postgresql:dbname=test;@blob=bytea' \
	# 'odbc:Driver=MySQL;UID=root;PWD=root;Database=test;@engine=mysql' \
	# 'odbc:Driver=PostgreSQL ANSI;Database=test;@engine=postgresql' \
	# 'odbc:Driver=Sqlite3;Database=/tmp/test.db;@engine=sqlite3' \

do
	for SUFFIX in '' ';@use_prepared=off' ';@pool_size=5' ';@use_prepared=off;@pool_size=5'
	do
		run_test "$TEST_DIR/test_backend" "$STR$SUFFIX"
		run_test "$TEST_DIR/test_basic" "$STR$SUFFIX"
	done
done

run_test "$TEST_DIR/test_caching"
run_test "$TEST_DIR/test_perf" "mysql:user=reza;password=r;database=reza;opt_reconnect=1;opt_read_timeout=10;opt_compress=1;@pool_size=15"

