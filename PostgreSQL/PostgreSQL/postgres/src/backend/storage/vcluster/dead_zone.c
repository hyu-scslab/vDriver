/*-------------------------------------------------------------------------
 *
 * dead_zone.c
 *
 * Dead Zone Implementation
 *
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/storage/vcluster/dead_zone.c
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
#include "storage/procarray.h"
#include "storage/lwlock.h"
#include "postmaster/fork_process.h"
#include "miscadmin.h"
#include "postmaster/postmaster.h"
#include "tcop/tcopprot.h"
#include "storage/sinvaladt.h"
#include "libpq/pqsignal.h"

#include "storage/vcluster.h"
#include "storage/thread_table.h"
#include "storage/vchain_hash.h"
#include "storage/vchain.h"
#ifdef HYU_LLT_STAT
#include "storage/vstatistic.h"
#endif

#include "storage/dead_zone.h"



/* dead zone in shared memory*/
DeadZoneDesc*	dead_zone_desc;

/* local functions */
static void DeadZoneUpdaterProcessMain(void);
static TransactionId MinInSnapshot(
		SnapshotTableNode*	snapshot,
		TransactionId		after,
		TransactionId		prev_snapshot_owner);
static void CalculateDeadZone(
		DeadZoneDesc*	desc,
		SnapshotTable	table);

static bool IsInDeadZone(TransactionId xmin, TransactionId xmax);



/*
 * DeadZoneShmemSize
 *
 * compute the size of shared memory for the dead zone
 */
Size
DeadZoneShmemSize(void)
{
	Size		size = 0;

	size = add_size(size, sizeof(DeadZoneDesc));

	return size;
}

/*
 * DeadZoneInit
 *
 * Initialize shared data structures related to dead zone in shared memory
 */
void
DeadZoneInit(void)
{
	bool		foundDesc;

	//assert(PROCARRAY_MAXPROCS < THREAD_TABLE_SIZE);

	dead_zone_desc = (DeadZoneDesc*)
		ShmemInitStruct("Dead Zone Descriptor",
						sizeof(DeadZoneDesc), &foundDesc);

	dead_zone_desc->cnt = 0;
}

/*
 * StartDeadZoneUpdater
 *
 * Fork dead zone updater process.
 */
pid_t
StartDeadZoneUpdater(void)
{
	pid_t		updater_pid;

	/* Start a cutter process. */
	updater_pid = fork_process();
	if (updater_pid == -1) {
		/* error */
	}
	else if (updater_pid == 0) {
		/* child */
		DeadZoneUpdaterProcessMain(); /* no return */
	}
	else {
		/* parent */
	}

	return updater_pid;
}

/*
 * MinInSnapshot
 *
 * Minimum xmin in snapshot bigger than after.
 */
static TransactionId
MinInSnapshot(SnapshotTableNode*	snapshot,
			  TransactionId			after,
			  TransactionId			prev_snapshot_owner)
{
	TransactionId	min_xid;
	TransactionId	xid;

	if (snapshot->cnt == 0) {
		return 0;
	}

	min_xid = snapshot->xmax;
	for (int i = 0; i < snapshot->cnt; i++) {
		xid = snapshot->snapshot[i];
		if (xid > after && xid < min_xid) {
			/* if the xid is the owner of the previous snapshot, skip it */
			if (xid == prev_snapshot_owner) continue;

			min_xid = xid;
		}
	}

	return min_xid;
}

/*
 * CalculateDeadZone
 *
 * Calculate dead zone by snapshot table.
 */
static void
CalculateDeadZone(DeadZoneDesc*	desc,
				  SnapshotTable	table)
{
	SnapshotTableNode	temp; /* for swap */
	TransactionId		min_xmax;
	int					min_index;
	SnapshotTableNode*	prev_snapshot;
	SnapshotTableNode*	next_snapshot;
	TransactionId		left;
	TransactionId		right;

	/* Sort snapshot table by xmax. */
	/* selection sort */
	for (int i = 0; i < THREAD_TABLE_SIZE; i++) {
		min_index = i;
		min_xmax = PG_UINT32_MAX;
		for (int j = i; j < THREAD_TABLE_SIZE; j++) {
			if (table[j].cnt == 0 && !TransactionIdIsValid(table[j].xmax))
				/* except empty node */
				continue;
			if (table[j].xmax < min_xmax) {
				min_xmax = table[j].xmax;
				min_index = j;
			}
			else if (table[j].xmax == min_xmax &&
					table[j].cnt > table[min_index].cnt)
			{
				/*
				 * xmax of two snapshot is same, but we need to determine the
				 * order between them. Transaction id larger than xmax might
				 * be already ignored, so if the xcnt of the transaction is
				 * smaller then it is late snapshot than another because
				 * someone could be committed out between them.
				 */
				min_xmax = table[j].xmax;
				min_index = j;
			}
		}

		if (i != min_index) {
			temp = table[i];
			table[i] = table[min_index];
			table[min_index] = temp;
		}
		if (min_xmax == PG_UINT32_MAX) {
			/* Nothing to sort anymore. */
			break;
		}
	}

	/* Calculate first zone. */
	if (table[0].cnt == 0 && !TransactionIdIsValid(table[0].xmax)) {
		/* No snapshot */
		desc->cnt = 0;
		return;
	}

	desc->dead_zones[0].left = 0;
	desc->dead_zones[0].right = MinInSnapshot(&table[0], 0, 0);
	desc->cnt = 1;

	/* Calculate other zones. */
	for (int i = 1; i < THREAD_TABLE_SIZE; i++) {
		prev_snapshot = &table[i - 1];
		next_snapshot = &table[i];
		if (next_snapshot->cnt == 0) {
			break;
		}

		left = prev_snapshot->xmax;
		right = MinInSnapshot(next_snapshot, left, prev_snapshot->owner);

		desc->dead_zones[i].left = left;
		desc->dead_zones[i].right = right;

		desc->cnt = i + 1;
	}
}

/*
 * SetDeadZone
 *
 * Calculate dead zone and notice it.
 */
void
SetDeadZone(void)
{
	SnapshotTable table;
	DeadZoneDesc* local_dead_zone_desc;

	/* Copy snapshot table to my local table. */
	table = AllocSnapshotTable();
	CopySnapshotTable(table);

	/* Let's calculate dead zone. */
	local_dead_zone_desc = 
		(DeadZoneDesc*) malloc(sizeof(DeadZoneDesc));
	CalculateDeadZone(local_dead_zone_desc, table);
#if 0 // #ifdef HYU_LLT_STAT
	elog(WARNING, "HYU_LLT : dead zone\n"
			"HYU_LLT\n"
			"HYU_LLT\n"
			"HYU_LLT         %10u %10u (%10u)\n"
			"HYU_LLT         %10u %10u (%10u)\n"
			"HYU_LLT         %10u %10u (%10u)\n"
			"HYU_LLT         %10u %10u (%10u)\n"
			"HYU_LLT         %10u %10u (%10u)\n"
			"HYU_LLT         %10u %10u (%10u)\n"
			"HYU_LLT\n"
			"HYU_LLT\n",
			local_dead_zone_desc->dead_zones[0].left, local_dead_zone_desc->dead_zones[0].right,
			local_dead_zone_desc->dead_zones[0].right - local_dead_zone_desc->dead_zones[0].left,
			local_dead_zone_desc->dead_zones[1].left, local_dead_zone_desc->dead_zones[1].right,
			local_dead_zone_desc->dead_zones[1].right - local_dead_zone_desc->dead_zones[1].left,
			local_dead_zone_desc->dead_zones[2].left, local_dead_zone_desc->dead_zones[2].right,
			local_dead_zone_desc->dead_zones[2].right - local_dead_zone_desc->dead_zones[2].left,
			local_dead_zone_desc->dead_zones[3].left, local_dead_zone_desc->dead_zones[3].right,
			local_dead_zone_desc->dead_zones[3].right - local_dead_zone_desc->dead_zones[3].left,
			local_dead_zone_desc->dead_zones[4].left, local_dead_zone_desc->dead_zones[4].right,
			local_dead_zone_desc->dead_zones[4].right - local_dead_zone_desc->dead_zones[4].left,
			local_dead_zone_desc->dead_zones[5].left, local_dead_zone_desc->dead_zones[5].right,
			local_dead_zone_desc->dead_zones[5].right - local_dead_zone_desc->dead_zones[5].left
			);
#endif

	/* Notice new dead zone. */
	LWLockAcquire(DeadZoneLock, LW_EXCLUSIVE);
	*dead_zone_desc = *local_dead_zone_desc;
	LWLockRelease(DeadZoneLock);

	/* Free local data. */
	FreeSnapshotTable(table);
	free(local_dead_zone_desc);
}

/*
 * VersionChainIsInDeadZone
 *
 * Visibility check for all versions in a version chain.
 * If all versions are all invisible, return true.
 */
bool
VersionChainIsInDeadZone(Oid rel_node,
                         PrimaryKey primary_key)
{
	LWLock			*partition_lock;
	uint32			hashcode;
	dsa_pointer		dsap_chain;
	VLocator		*chain;
	VLocator		*locator;
	VChainTag		chain_tag;

	chain_tag.rel_node = rel_node;
	chain_tag.primary_key = primary_key;

	/* Get hash code for the primary key */
	hashcode = VChainHashCode(&chain_tag);
	partition_lock = VChainMappingPartitionLock(hashcode);

	/* Acquire partition lock for the primary key with shared mode */
	LWLockAcquire(partition_lock, LW_SHARED);
	if (!VChainHashLookup(&chain_tag, hashcode, &dsap_chain))
	{
		/* There is no hash entry for the primary key in the vchain hash */
		LWLockRelease(partition_lock);
		return true;
	}
	LWLockRelease(partition_lock);

	/* Set epoch */
	SetTimestamp();
	pg_memory_barrier();

	chain = (VLocator *)dsa_get_address(dsa_vcluster, dsap_chain);
	if (chain->dsap_prev == chain->dsap)
	{
		/* There is a hash entry, but the version chain is empty */

		/* Clear epoch */
		pg_memory_barrier();
		ClearTimestamp();

		return true;
	}

	/*
	 * Now we have the hash entry (dummy node) that indicates the
	 * head/tail of the version chain.
	 * Try to find any visible version from the recent version (tail)
	 */
	locator = (VLocator *)dsa_get_address(dsa_vcluster, chain->dsap_prev);
	while (locator->dsap != chain->dsap)
	{
		if (!RecIsInDeadZone(locator->xmin, locator->xmax))
		{
			/* This version is visible for some transactions. */

			/* Clear epoch */
			pg_memory_barrier();
			ClearTimestamp();

			return false;
		}
		locator = (VLocator *)dsa_get_address(
				dsa_vcluster, locator->dsap_prev);
	}

	/* There are no visible version for all live transactions. */

	/* Clear epoch */
	pg_memory_barrier();
	ClearTimestamp();

	return true;
}

/*
 * RecIsInDeadZone
 *
 * Wrapper function for record.
 */
bool
RecIsInDeadZone(TransactionId xmin, TransactionId xmax)
{
	if (xmax < RecentGlobalXmin + vacuum_defer_cleanup_age) {
		/* It's optimized-zone1-range. */
		return true;
	}
	if (IsInDeadZone(xmin, xmax)) {
		/* Calculated-dead-zone. */
		return true;
	}

	return false;
}

/*
 * RecIsInDeadZone
 *
 * Wrapper function for segment.
 */
bool
SegIsInDeadZone(TransactionId xmin, TransactionId xmax)
{
	if (xmax < RecentGlobalXmin + vacuum_defer_cleanup_age) {
		/* It's optimized-zone1-range. */
		return true;
	}
	if (IsInDeadZone(xmin, xmax)) {
		/* Calculated-dead-zone. */
		return true;
	}

	return false;
}

/*
 * IsInDeadZone
 *
 * Decide whether this version(record, segment desc) is dead or not.
 * We only judge by dead zone, xmin and xmax.
 */
static bool
IsInDeadZone(TransactionId xmin, TransactionId xmax)
{
	bool ret = false;

	assert(xmin <= xmax);

	LWLockAcquire(DeadZoneLock, LW_SHARED);
	for (int i = 0; i < dead_zone_desc->cnt; i++) {
		DeadZone* dead_zone = &dead_zone_desc->dead_zones[i];
		if (xmin > dead_zone->left && xmax < dead_zone->right) {
			ret = true;
			break;
		}
	}
	LWLockRelease(DeadZoneLock);

	return ret;
}

/*
 * DeadZoneUpdaterProcessMain
 *
 * Main function of dead zone updater process.
 */
static void
DeadZoneUpdaterProcessMain(void)
{
	/* just copy routine to hear from other process-generation-code */
	/* ex) StartAutoVacLauncher */
	InitPostmasterChild();
	ClosePostmasterPorts(false);
	SetProcessingMode(InitProcessing);
	pqsignal(SIGQUIT, my_quick_die);
	pqsignal(SIGTERM, my_quick_die);
	pqsignal(SIGKILL, my_quick_die);
	pqsignal(SIGINT, my_quick_die);
	PG_SETMASK(&UnBlockSig);
	BaseInit();
	InitProcess();

	/* Main routine */
	for (;;) {
		//elog(WARNING, "HYU_LLT : updater looping..");
		SetDeadZone();
		sleep(INTERVAL_UPDATE_DEADZONE);
	}
}


#endif /* HYU_LLT */
