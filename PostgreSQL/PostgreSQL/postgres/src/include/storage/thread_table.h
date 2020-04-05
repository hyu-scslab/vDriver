/*-------------------------------------------------------------------------
 *
 * thread_table.h
 *	  thread table
 *
 *
 *
 * src/include/storage/thread_table.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef THREAD_TABLE_H
#define THREAD_TABLE_H

#include "c.h"
#include "utils/dsa.h"
#include "utils/snapshot.h"
#include "utils/snapmgr.h"
#include "utils/timestamp.h"


/* Size of thread table : Max number of simultaneous running processes */
/* This value is from procArray->maxProcs(122) */
#define THREAD_TABLE_SIZE	(1300)

/* Max number of snaped-active trx ids */
#define SNAPSHOT_SIZE		(1300)


#define TS_NONE				(0)

typedef struct {
	/* FIXME: is need to use volatile?? */
	volatile TimestampTz		timestamp;
} TimestampTableNode;

typedef TimestampTableNode*	TimestampTable;



typedef struct {
	/* Snapshot table is protected by ProcArrayLock. */
	/* It's safe to update snapshot with shared lock of ProcArrayLock
	 * because each slot of table is accessed by only one process. */
	/* But, if you want to copy this table to other space, you may
	   have to acquire exclusive lock of ProcArrayLock. */
	TransactionId				snapshot[SNAPSHOT_SIZE];
	int							cnt; /* size of snapshot */
	TransactionId				xmax;
	TransactionId				owner;
} SnapshotTableNode;

typedef SnapshotTableNode*		SnapshotTable;


/* Thread table descriptor */
/* Each node of all table is assigned to each process. */
/* It makes that processes have no write-write conflict. */
typedef struct {
	TimestampTableNode	timestamp_table[THREAD_TABLE_SIZE];
	SnapshotTableNode	snapshot_table[THREAD_TABLE_SIZE];
} ThreadTableDesc;




extern ThreadTableDesc*	thread_table_desc;

extern Size ThreadTableShmemSize(void);
extern void ThreadTableInit(void);

extern void SetTimestamp(void);
extern void ClearTimestamp(void);
extern TimestampTz GetMinimumTimestamp(void);

extern void SetSnapshot(TransactionId*	snapshot,
						int 			cnt,
						TransactionId	xmax);
extern void SetSnapshotOwner(TransactionId owner);
extern void ClearSnapshot(void);
extern void CopySnapshotTable(SnapshotTable table);
extern SnapshotTable AllocSnapshotTable(void);
extern void FreeSnapshotTable(SnapshotTable table);


#endif							/* THREAD_TABLE_H */
