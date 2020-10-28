#
# Loop body of binlog_truncate_multi_engine.test
# Parameters:
#   $debug_sync_action  describes debug-sync actions
#   $restart_options    the server restart options
#   $test_outcome       summary of extected results
#   $MYSQLD_DATADIR     

--echo #
--echo #
--echo # Case $case
--echo #
RESET MASTER;
FLUSH LOGS;
SET GLOBAL max_binlog_size= 4096;

connect(con1,localhost,root,,);
#--connection con1
--echo List of binary logs before rotation
--source include/show_binary_logs.inc
INSERT INTO t1 VALUES (1, REPEAT("x", 1));
INSERT INTO t2 VALUES (1, REPEAT("x", 1));
BEGIN;
  INSERT INTO t1 VALUES (2, REPEAT("x", 4100));
  INSERT INTO t2 VALUES (2, REPEAT("x", 4100));

--eval SET DEBUG_SYNC= $debug_sync_action
send COMMIT;

--connection default
SET DEBUG_SYNC= "now WAIT_FOR con1_ready";
--echo List of binary logs after rotation
--source include/show_binary_logs.inc
--write_file $MYSQLTEST_VARDIR/tmp/mysqld.1.expect
wait
EOF

# Terminate server
if (!$kill_server)
{
  --echo # server is shut down
  --exec echo "wait" > $MYSQLTEST_VARDIR/tmp/mysqld.1.expect
  --shutdown_server
}
if ($kill_server)
{
  --echo # server is killed
  --source include/kill_mysqld.inc
}
--source include/wait_until_disconnected.inc

--echo *** After server is terminated (killed (A,B) or shutdown (C))
--list_files $MYSQLD_DATADIR/ master-bin.0*

#
--echo # Server recovery restart
#
--error 1
--exec $MYSQLD_LAST_CMD $restart_options > $MYSQLTEST_VARDIR/log/mysqld.1.err 2>&1
#--exec echo restart:$restart_options > $MYSQLTEST_VARDIR/tmp/mysqld.1.expect
#--source include/wait_until_disconnected.inc#
## or --sleep 0.1

--echo # normal restart
--append_file $MYSQLTEST_VARDIR/tmp/mysqld.1.expect
restart:
EOF

--connection default
--enable_reconnect
--source include/wait_until_connected_again.inc

--echo #
--echo # *** Summary: $test_outcome:
--echo #
SELECT COUNT(*) FROM t1;
SELECT COUNT(*) FROM t2;
SELECT @@GLOBAL.gtid_current_pos;
--echo List of binary logs at the end of the tests
--source include/show_binary_logs.inc
--echo # ***
# cleanup
DELETE FROM t1;
DELETE FROM t2;
--disconnect con1
--echo #

