/*
   Copyright (c) 2000, 2019, Oracle and/or its affiliates.
   Copyright (c) 2010, 2020, MariaDB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1335  USA
*/

/* External interfaces to ddl log functions */

#ifndef DDL_LOG_INCLUDED
#define DDL_LOG_INCLUDED

enum ddl_log_entry_code
{
  /*
    DDL_LOG_UNKOWN
      Here mainly to detect blocks that are all zero

    DDL_LOG_EXECUTE_CODE:
      This is a code that indicates that this is a log entry to
      be executed, from this entry a linked list of log entries
      can be found and executed.
    DDL_LOG_ENTRY_CODE:
      An entry to be executed in a linked list from an execute log
      entry.
    DDL_IGNORE_LOG_ENTRY_CODE:
      An entry that is to be ignored
  */
  DDL_LOG_UNKNOWN= 0,
  DDL_LOG_EXECUTE_CODE= 1,
  DDL_LOG_ENTRY_CODE= 2,
  DDL_IGNORE_LOG_ENTRY_CODE= 3,
  DDL_LOG_ENTRY_CODE_LAST= 4
};


/*
  When adding things below, also add an entry to ddl_log_entry_phases in
  ddl_log.cc
*/

enum ddl_log_action_code
{
  /*
    The type of action that a DDL_LOG_ENTRY_CODE entry is to
    perform.
  */

  /* Delete a .frm file or a table in the partition engine */
  DDL_LOG_DELETE_ACTION= 0,

  /* Rename a .frm fire a table in the partition engine */
  DDL_LOG_RENAME_ACTION= 1,

  /*
    Rename an entity after removing the previous entry with the
    new name, that is replace this entry.
  */
  DDL_LOG_REPLACE_ACTION= 2,

  /* Exchange two entities by renaming them a -> tmp, b -> a, tmp -> b */
  DDL_LOG_EXCHANGE_ACTION= 3,
  /*
    log do_rename(): Rename of .frm file, table, stat_tables and triggers
  */
  DDL_LOG_RENAME_TABLE_ACTION= 4,
  DDL_LOG_RENAME_VIEW_ACTION= 5,
  DDL_LOG_LAST_ACTION                           /* End marker */
};


/* Number of phases for each ddl_log_action_code */
extern const uchar ddl_log_entry_phases[DDL_LOG_LAST_ACTION];


enum enum_ddl_log_exchange_phase {
  EXCH_PHASE_NAME_TO_TEMP= 0,
  EXCH_PHASE_FROM_TO_NAME= 1,
  EXCH_PHASE_TEMP_TO_FROM= 2
};

enum enum_ddl_log_rename_table_phase {
  DDL_RENAME_PHASE_TRIGGER= 0,
  DDL_RENAME_PHASE_STAT,
  DDL_RENAME_PHASE_TABLE,
};

/*
  Setting ddl_log_entry.phase to this has the same effect as setting
  action_type to DDL_IGNORE_LOG_ENTRY_CODE
*/

#define DDL_LOG_FINAL_PHASE ((uchar) 0xff)

typedef struct st_ddl_log_entry
{
  const char *name;
  const char *from_name;
  const char *handler_name;
  const char *tmp_name;
  /* At some point all strings above should be LEX_CSTRING's */
  LEX_CSTRING db, from_db;

  ulonglong xid;
  uint next_entry;
  uint entry_pos;                      // Set by write_dll_log_entry()
  // bool temporary;                      // If drop of temporary table
  enum ddl_log_entry_code entry_type;  // Set automatically
  enum ddl_log_action_code action_type;
  /*
    Most actions have only one phase. REPLACE does however have two
    phases. The first phase removes the file with the new name if
    there was one there before and the second phase renames the
    old name to the new name.
  */
  uchar phase;                         // set automatically
} DDL_LOG_ENTRY;

typedef struct st_ddl_log_memory_entry
{
  uint entry_pos;
  struct st_ddl_log_memory_entry *next_log_entry;
  struct st_ddl_log_memory_entry *prev_log_entry;
  struct st_ddl_log_memory_entry *next_active_log_entry;
} DDL_LOG_MEMORY_ENTRY;


typedef struct st_ddl_log_state
{
  uint next_entry, execute_entry_pos;
  /* List of ddl log entries */
  DDL_LOG_MEMORY_ENTRY *list;
  /* One execute entry per list */
  DDL_LOG_MEMORY_ENTRY *execute_entry;
} DDL_LOG_STATE;


/* These functions are for recovery */
bool ddl_log_initialize();
void ddl_log_release();
bool ddl_log_close_binlogged_events(HASH *xids);
int ddl_log_execute_recovery();

/* functions for updating the ddl log */
bool ddl_log_write_entry(DDL_LOG_ENTRY *ddl_log_entry,
                           DDL_LOG_MEMORY_ENTRY **active_entry);

bool ddl_log_write_execute_entry_internal(uint first_entry,
                                          bool complete,
                                          DDL_LOG_MEMORY_ENTRY **active_entry);
static inline bool
ddl_log_write_execute_entry(uint first_entry,
                            DDL_LOG_MEMORY_ENTRY **active_entry)
{
  return ddl_log_write_execute_entry_internal(first_entry, 0, active_entry);
}

static inline bool
ddl_log_disable_execute_entry(DDL_LOG_MEMORY_ENTRY **active_entry)
{
  return ddl_log_write_execute_entry_internal(0, TRUE, active_entry);
}

void ddl_log_complete(DDL_LOG_STATE *ddl_log_state);
void ddl_log_revert(THD *thd, DDL_LOG_STATE *ddl_log_state);

bool ddl_log_update_phase(DDL_LOG_STATE *entry, uchar phase);
bool ddl_log_update_xid(DDL_LOG_STATE *state, ulonglong xid);
bool ddl_log_disable_entry(DDL_LOG_STATE *state);
bool ddl_log_increment_phase(uint entry_pos);
void ddl_log_release_memory_entry(DDL_LOG_MEMORY_ENTRY *log_entry);
bool ddl_log_sync();
bool ddl_log_execute_entry(THD *thd, uint first_entry);

void ddl_log_release_entries(DDL_LOG_STATE *ddl_log_state);
bool ddl_log_rename_table(THD *thd, DDL_LOG_STATE *ddl_state,
                          handlerton *hton,
                          const LEX_CSTRING *org_db,
                          const LEX_CSTRING *org_alias,
                          const LEX_CSTRING *new_db,
                          const LEX_CSTRING *new_alias);
bool ddl_log_rename_view(THD *thd, DDL_LOG_STATE *ddl_state,
                         const LEX_CSTRING *org_db,
                         const LEX_CSTRING *org_alias,
                         const LEX_CSTRING *new_db,
                         const LEX_CSTRING *new_alias);

extern mysql_mutex_t LOCK_gdl;
#endif /* DDL_LOG_INCLUDED */
