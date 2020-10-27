#
# Loop body of binlog_truncate_multi_engine.test
# Parameters:
#   $debug_sync_action  describes debug-sync actions
#

RESET MASTER;
FLUSH LOGS;

connect(con1,localhost,root,,);
#--connection con1
--echo "List of binary logs before rotation"
--source include/show_binary_logs.inc
BEGIN;
  INSERT INTO t2 VALUES (1, REPEAT("x", 4100));
  INSERT INTO t1 VALUES (2, REPEAT("x", 4100));

--echo # Case A. neither of the engines commits
--eval SET DEBUG_SYNC= $debug_sync_action
send COMMIT;

--connection default
SET DEBUG_SYNC= "now WAIT_FOR con1_ready";
--echo "List of binary logs after rotation"
--source include/show_binary_logs.inc
--write_file $MYSQLTEST_VARDIR/tmp/mysqld.1.expect
wait
EOF

--source include/kill_mysqld.inc
--source include/wait_until_disconnected.inc

#
# Server restart
#
--append_file $MYSQLTEST_VARDIR/tmp/mysqld.1.expect
restart: $restart_options
EOF
--source include/wait_until_disconnected.inc

--append_file $MYSQLTEST_VARDIR/tmp/mysqld.1.expect
restart:
EOF

--connection default
--enable_reconnect
--source include/wait_until_connected_again.inc

--echo $test_outcome
SELECT COUNT(*) FROM t1;
SELECT COUNT(*) FROM t2;
SELECT @@GLOBAL.gtid_current_pos;

--echo "List of binary logs after the tests"
--source include/show_binary_logs.inc

# cleanup
DELETE FROM t1;
DELETE FROM t2;
--disconnect con1
