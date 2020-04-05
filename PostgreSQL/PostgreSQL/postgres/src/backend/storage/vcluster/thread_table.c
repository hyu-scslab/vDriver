/*-------------------------------------------------------------------------
 *
 * thread_table.c
 *
 * Thread Table Implementation
 *
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/storage/vcluster/thread_table.c
 *
 *-------------------------------------------------------------------------
 */
#ifdef HYU_LLT
#include "postgres.h"

#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "storage/lwlock.h"
#include "storage/proc.h"
#include "storage/shmem.h"
#include "storage/vcluster.h"

#include "storage/thread_table.h"

/* thread table in shared memory*/
ThreadTableDesc*	thread_table_desc;

/*
 * ThreadTableShmemSize
 *
 * compute the size of shared memory for the thread table
 */
Size
ThreadTableShmemSize(void)
{
	Size		size = 0;

	size = add_size(size, sizeof(ThreadTableDesc));

	return size;
}

/*
 * ThreadTableInit
 *
 * Initialize shared data structures related to thread table in shared memory
 */
void
ThreadTableInit(void)
{
	bool		foundDesc;

	thread_table_desc = (ThreadTableDesc*)
		ShmemInitStruct("Thread Table Descriptor",
						sizeof(ThreadTableDesc), &foundDesc);

	/* Initialize thread table */
	for (int i = 0; i < THREAD_TABLE_SIZE; i++)
	{
		/* Init timestamp table */
		thread_table_desc->timestamp_table[i].timestamp = TS_NONE;

		/* Init snapshot table */
		for (int j = 0; j < SNAPSHOT_SIZE; j++) {
			thread_table_desc->snapshot_table[i].snapshot[j] = 0;
		}
		thread_table_desc->snapshot_table[i].xmax = 0;
	}
}

/*
 * SetSnapshot
 *
 * Copy snapshot on snap table.
 * Caller HAVE TO hold ProcArrayLock(shared).
 */
void
SetSnapshot(TransactionId*	snapshot,
		    int				cnt,
			TransactionId	xmax)
{
	int					index;
	SnapshotTable		snapshot_table;
	SnapshotTableNode*	node;

	assert(cnt <= SNAPSHOT_SIZE);

	index = MyProc->pgprocno;
	snapshot_table = thread_table_desc->snapshot_table;
	node = &snapshot_table[index];

	/* Copy snapshot. */
	for (int i = 0; i < cnt; i++) {
		node->snapshot[i] = snapshot[i];
	}
	node->cnt = cnt;
	node->xmax = xmax;
}

/*
 * SetSnapshotOwner
 *
 * Set snapshot owner's transaction id for the snapshot in snapshot table
 * NOTE: we call this function without acquiring ProcArrayLock
 */
void
SetSnapshotOwner(TransactionId owner)
{
	int					index;
	SnapshotTable		snapshot_table;
	SnapshotTableNode*	node;

	index = MyProc->pgprocno;
	snapshot_table = thread_table_desc->snapshot_table;
	node = &snapshot_table[index];

	node->owner = owner;
}

/*
 * ClearSnapshot
 *
 * Clear snapshot.
 * Caller HAVE TO hold ProcArrayLock(shared).
 */
void
ClearSnapshot(void)
{
	int					index;
	SnapshotTable		snapshot_table;
	SnapshotTableNode*	node;

	index = MyProc->pgprocno;
	snapshot_table = thread_table_desc->snapshot_table;
	node = &snapshot_table[index];

	/* Clear snapshot. */
	/* Maybe it is sufficient to update only cnt. */
	node->cnt = 0;
	node->xmax = InvalidTransactionId;
}

/*
 * AllocSnapshotTable
 *
 * Allocation variable for snapshot table.
 * It may be used to copy snapshot table.
 */
SnapshotTable
AllocSnapshotTable(void)
{
	SnapshotTable table = (SnapshotTable) malloc(
			sizeof(SnapshotTableNode) * THREAD_TABLE_SIZE);
	
	return table;
}

/*
 * FreeSnapshotTable
 *
 * Free snapshot table.
 */
void
FreeSnapshotTable(SnapshotTable table)
{
	free(table);
}

/*
 * CopySnapshotTable
 *
 * Copy snapshot table to table(parameter).
 * ProcArrayLock(exclusive) is holded in this function.
 */
void
CopySnapshotTable(SnapshotTable table)
{
	SnapshotTable		snapshot_table;

	snapshot_table = thread_table_desc->snapshot_table;

	LWLockAcquire(ProcArrayLock, LW_EXCLUSIVE);

	/* FIXME: is it posibble??
	 * *table = *snapshot_table */
	for (int i = 0; i < THREAD_TABLE_SIZE; i++) {
		for (int j = 0; j < snapshot_table[i].cnt; j++) {
			table[i].snapshot[j] = snapshot_table[i].snapshot[j];
		}
		table[i].cnt = snapshot_table[i].cnt;
		table[i].xmax = snapshot_table[i].xmax;
		table[i].owner = snapshot_table[i].owner;
	}


	LWLockRelease(ProcArrayLock);
}

/*
 * SetTimestamp
 *
 * Set caller's timestamp on timestamp table.
 */
void
SetTimestamp(void)
{
	int index = MyProc->pgprocno;
	TimestampTable timestamp_table = thread_table_desc->timestamp_table;

	timestamp_table[index].timestamp = GetCurrentTimestamp();
}

/*
 * ClearTimestamp
 *
 * Clear caller's timestamp on timestamp table.
 */
void
ClearTimestamp(void)
{
	int index = MyProc->pgprocno;
	TimestampTable timestamp_table = thread_table_desc->timestamp_table;

	timestamp_table[index].timestamp = TS_NONE;
}

/*
 * GetMinimumTimestamp
 *
 * Return the minimum timestamp from timestamp table.
 * If timestamp of a object is smaller than GetMinimumTimestamp(), 
 * we can be sure that any other processes can't see the object.
 */
TimestampTz
GetMinimumTimestamp(void)
{
	TimestampTz min_ts;

	min_ts = GetCurrentTimestamp();
	for (int i = 0; i < THREAD_TABLE_SIZE; i++)
	{
		TimestampTz ts;
		ts = thread_table_desc->timestamp_table[i].timestamp;

		if (ts == TS_NONE)
			continue;

		if (ts < min_ts)
			min_ts = ts;
	}

	return min_ts;
}
#endif /* HYU_LLT */
