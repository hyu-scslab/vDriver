/*-------------------------------------------------------------------------
 *
 * vcluster.c
 *
 * Version Cluster Implementation
 *
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/storage/vcluster/vcluster.c
 *
 *-------------------------------------------------------------------------
 */
#ifdef HYU_LLT
#include "postgres.h"

#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "access/transam.h"
#include "storage/lwlock.h"
#include "storage/proc.h"
#include "postmaster/fork_process.h"
#include "storage/shmem.h"
#include "miscadmin.h"
#include "postmaster/postmaster.h"
#include "tcop/tcopprot.h"
#include "storage/ipc.h"
#include "storage/sinvaladt.h"
#include "libpq/pqsignal.h"
#include "storage/standby.h"

#include "storage/vcache.h"
#include "storage/vchain.h"
#include "storage/thread_table.h"
#include "storage/dead_zone.h"
#ifdef HYU_LLT_STAT
#include "storage/vstatistic.h"
#endif /* HYU_LLT_STAT */

#include "storage/vcluster.h"

#define AVG_STAT_WEIGHT					(0.9999)
#define CLASSIFICATION_THRESHOLD_COLD	(10)
#ifndef CLASSIFICATION_THRESHOLD_LLT	/* For configuration */
#define CLASSIFICATION_THRESHOLD_LLT	(200) //(40000)
#endif

/* vcluster descriptor in shared memory*/
VClusterDesc	*vclusters;

/* dsa_area of vcluster for this process */
dsa_area		*dsa_vcluster;

/* decls for local routines only used within this module */
static VCLUSTER_TYPE
VersionClassification(TransactionId xmin,
					  TransactionId xmax,
					  FullTransactionId nextFullId,
					  Snapshot snapshot);

static dsa_pointer AllocNewSegmentInternal(dsa_area *dsa,
										   VCLUSTER_TYPE type);
static dsa_pointer AllocNewSegment(VCLUSTER_TYPE type);
static void VClusterCutProcessMain(void);
static void GCProcessMain(void);

/*
 * VClusterShmemSize
 *
 * compute the size of shared memory for the vcluster
 */
Size
VClusterShmemSize(void)
{
	Size		size = 0;

	size = add_size(size, sizeof(VClusterDesc));

	size = add_size(size, VCacheShmemSize());

	size = add_size(size, VChainShmemSize());

	size = add_size(size, ThreadTableShmemSize());

	size = add_size(size, DeadZoneShmemSize());
#ifdef HYU_LLT_STAT
	size = add_size(size, VStatisticShmemSize());
#endif /* HYU_LLT_STAT */

	return size;
}

/*
 * VClusterShmemInit
 *
 * Initialize shared data structures related to vcluster in shared memory
 */
void
VClusterShmemInit(void)
{
	bool		foundDesc;

	VCacheInit();
	VChainInit();
	ThreadTableInit();
	DeadZoneInit();
#ifdef HYU_LLT_STAT
	VStatisticInit();
#endif /* HYU_LLT_STAT */
	
	vclusters = (VClusterDesc *)
		ShmemInitStruct("VCluster Descriptor",
						sizeof(VClusterDesc), &foundDesc);

	/* We use segment id from 1, 0 is reserved for exceptional cases */
	for (int i = 0; i < VCLUSTER_NUM; i++)
	{
		vclusters->reserved_seg_id[i] = i + 1;
	}
	pg_atomic_init_u32(&vclusters->next_seg_id, VCLUSTER_NUM + 1);
	
	assert(sizeof(VRecord) == VCLUSTER_TUPLE_SIZE);
}

/*
 * StartVCutter
 *
 * Fork vcutter process.
 */
pid_t
StartVCutter(void)
{
	pid_t		cutter_pid;

	/* Start a cutter process. */
	cutter_pid = fork_process();
	if (cutter_pid == -1) {
		/* error */
	}
	else if (cutter_pid == 0) {
		/* child */
		VClusterCutProcessMain(); /* no return */
	}
	else {
		/* parent */
	}

	return cutter_pid;
}

/*
 * StartGC
 *
 * Fork garbage collector process.
 */
pid_t
StartGC(void)
{
	pid_t		gc_pid;

	/* Start a garbage collector process. */
	gc_pid = fork_process();
	if (gc_pid == -1) {
		/* error */
	}
	else if (gc_pid == 0) {
		/* child */
		GCProcessMain(); /* no return */
	}
	else {
		/* parent */
	}

	return gc_pid;
}


/*
 * VClusterDsaInit
 *
 * Initialize a dsa area for vcluster.
 * This function should only be called after the initialization of
 * the dynamic shared memory facilities.
 */
void
VClusterDsaInit(void)
{
	dsa_area	*dsa;
	dsa_handle	handle;

	/*
	 * The first backend process creates the dsa area for vcluster,
	 * and another backend processes waits the creation and then attach to it.
	 */
	if (ProcGlobal->vcluster_dsa_handle == 0)
	{
		uint32 expected = 0;
		if (pg_atomic_compare_exchange_u32(
				(pg_atomic_uint32 *)(&ProcGlobal->vcluster_dsa_handle),
				&expected, UINT32_MAX))
		{
			/* This process is selected to create dsa_area itself */

			/* Initialize dsa area for vcluster */
			dsa = dsa_create(LWTRANCHE_VCLUSTER);
			handle = dsa_get_handle(dsa);
			dsa_pin(dsa);

			/*
			 * Pre-allocate and initialize the first segment descriptors, files
			 * for each vcluster.
			 * assign segment id 0, 1, 2 for HOT, COLD, LLT segment, respectively.
			 */
			/* FIXME: need to devide code between dsa init and initiate variables. */
			for (int i = 0; i < VCLUSTER_NUM; i++)
			{
				dsa_pointer		dsap_seg_desc;
				VSegmentDesc*	seg_desc;

				/* First available segment */
				vclusters->tail[i] = AllocNewSegmentInternal(dsa, i);
				vclusters->head[i] = vclusters->tail[i];

				/* A dummy node for garbage list. */
				/* Allocate a new segment descriptor in shared memory */
				dsap_seg_desc = dsa_allocate_extended(
						dsa, sizeof(VSegmentDesc), DSA_ALLOC_ZERO);
				seg_desc = (VSegmentDesc *)dsa_get_address(dsa, dsap_seg_desc);

				/* dummy node need only one variable "next". */
				seg_desc->next = 0;

				vclusters->garbage_list_head[i] = dsap_seg_desc;
				vclusters->garbage_list_tail[i] = dsap_seg_desc;
			}
			dsa_detach(dsa);

			pg_memory_barrier();

			ProcGlobal->vcluster_dsa_handle = handle;
		}
	}
	while (ProcGlobal->vcluster_dsa_handle == UINT32_MAX)
	{
		/*
		 * Another process is creating an initial dsa area for vcluster,
		 * so just wait it to finish and then attach to it.
		 */
		usleep(1);
	}

	VClusterAttachDsa();
}

/*
 * VClusterAttachDsat
 *
 * Attach to the dsa area for vcluster.
 */
void
VClusterAttachDsa(void)
{
	if (dsa_vcluster != NULL)
		return;
	
	dsa_vcluster = dsa_attach(ProcGlobal->vcluster_dsa_handle);
	dsa_pin_mapping(dsa_vcluster);
}

/*
 * VClusterDsaInit
 *
 * Detach from the dsa area for vcluster.
 */
void
VClusterDetachDsa(void)
{
	if (dsa_vcluster == NULL)
		return;

	dsa_detach(dsa_vcluster);
	dsa_vcluster = NULL;
}

#if 0
/*
 * VClusterLookupTuple
 *
 * Find a visible version tuple with given primary key and snapshot.
 * Returns true if found. Returns false if not found.
 */
bool
VClusterLookupTuple(Oid rel_node,
					PrimaryKey primary_key,
					Size size,
					Snapshot snapshot,
					void *ret_tuple)
{
	VLocator *locator;
	if (!VChainLookupLocator(rel_node, primary_key, snapshot, &locator))
	{
		/* Couldn't find the visible locator for the primary key */
		return false;
	}

	VCacheReadTuple(locator->seg_id,
					locator->seg_offset,
					size,
					ret_tuple);
	
	return true;
}
#endif
/*
 * VClusterLookupTuple
 *
 * Find a visible version tuple with given primary key and snapshot.
 * Set ret_tuple to the pointer of the tuple in the vcache entry, and
 * return the pinned vcache id. Caller should unpin the cache after using.
 * Return InvalidVCache if not found.
 */
int
VClusterLookupTuple(Oid rel_node,
					PrimaryKey primary_key,
					Snapshot snapshot,
					void **ret_tuple)
{
	VLocator   *locator;
	int			cache_id;

	if (!VChainLookupLocator(rel_node, primary_key, snapshot, &locator))
	{
		/* Couldn't find the visible locator for the primary key */
		return InvalidVCache;
	}

	cache_id = VCacheReadTupleRef(locator->seg_id,
								  locator->seg_offset,
								  ret_tuple);
	return cache_id;
}
/*
 * VClusterAppendTuple
 *
 * Append a given tuple into the vcluster whose type is cluster_type
 */
void
VClusterAppendTuple(Oid rel_node,
					PrimaryKey primary_key,
					TransactionId xmin,
					TransactionId xmax,
					Snapshot snapshot,
					Size tuple_size,
					const void *tuple)
{
	VSegmentDesc 	*seg_desc;
	int				alloc_seg_offset;
	dsa_pointer		dsap_new_seg;
	VSegmentId		reserved_seg_id;
	int				vidx;
	VCLUSTER_TYPE	cluster_type;

	/*
	 * Alined size with power of 2. This is needed because
	 * the current version only support fixed size tuple with power of 2
	 */
	Size			aligned_tuple_size;
	
	Assert(dsa_vcluster != NULL);

	/* Update the statistics for version classification */
	VClusterUpdateVersionStatistics(xmin, xmax);

	/* Decide the version cluster type */
	cluster_type = VersionClassification(
			xmin, xmax, ShmemVariableCache->nextFullXid, snapshot);

	Assert(cluster_type == VCLUSTER_HOT ||
		   cluster_type == VCLUSTER_COLD ||
		   cluster_type == VCLUSTER_LLT);
#ifdef HYU_LLT_STAT
	__sync_fetch_and_add(&vstatistic_desc->cnt_inserted, 1);
    __sync_fetch_and_add(&vstatistic_desc->cnt_inserted_cluster[cluster_type], 1);
#endif

	/* First prune. */
	if (RecIsInDeadZone(xmin, xmax)) {
#ifdef HYU_LLT_STAT
		__sync_fetch_and_add(&vstatistic_desc->cnt_first_prune, 1);
        __sync_fetch_and_add(&vstatistic_desc->cnt_first_prune_cluster[cluster_type], 1);
#endif
		return;
	}
#ifdef HYU_LLT_STAT
	__sync_fetch_and_add(&vstatistic_desc->cnt_after_first_prune, 1);
    __sync_fetch_and_add(&vstatistic_desc->cnt_after_first_prune_cluster[cluster_type], 1);
#endif

	aligned_tuple_size = 1 << my_log2(tuple_size);

retry:

	/* Set epoch */
	SetTimestamp();
	pg_memory_barrier();

	/*
	 * The order of reading the segment id and the reserved segment id
	 * is important for making happen-before relation.
	 * If we read the reserved segment id first, then the segment id and
	 * the reserved segment id could be same.
	 */
	seg_desc = (VSegmentDesc *)dsa_get_address(
			dsa_vcluster, vclusters->tail[cluster_type]);
	pg_memory_barrier();
	reserved_seg_id = vclusters->reserved_seg_id[cluster_type];
	
	/* Pre-check whether the current segment is full */
	if (pg_atomic_read_u32(&seg_desc->seg_offset) > VCLUSTER_SEGSIZE)
	{
		/*
		 * Current segment is full, someone might(must) opening a
		 * new segmnet. Backoff and waiting it.
		 */
		pg_usleep(1000L);
		goto retry;
	}

	/* Allocate tuple space with aligned size with power of 2 */
	alloc_seg_offset = pg_atomic_fetch_add_u32(
			&seg_desc->seg_offset, aligned_tuple_size);

	if (alloc_seg_offset + aligned_tuple_size <= VCLUSTER_SEGSIZE)
	{
		/* Success to allocate segment space */

		/*
		 * NOTE: We are assuming that the tuple would not be overlapped
		 * between two pages
		 */
		VCacheAppendTuple(seg_desc->seg_id,
						  reserved_seg_id,
						  alloc_seg_offset,
						  tuple_size,
						  tuple,
                          cluster_type,
						  xmin,
						  xmax);
		
		/* Set vlocator of the version tuple */
		vidx = alloc_seg_offset / aligned_tuple_size;
		seg_desc->locators[vidx].xmin = xmin;
		seg_desc->locators[vidx].xmax = xmax;
		seg_desc->locators[vidx].seg_id = seg_desc->seg_id;
		seg_desc->locators[vidx].seg_offset = alloc_seg_offset;
		seg_desc->locators[vidx].dsap_prev = 0;
		seg_desc->locators[vidx].dsap_next = 0;

		/* Append the locator into the version chain */
		VChainAppendLocator(rel_node, primary_key, &seg_desc->locators[vidx]);

		if (VCLUSTER_SEG_NUM_ENTRY ==
				__sync_add_and_fetch(&seg_desc->counter, 1)) {
			/* You are last one which is inserting in this seg desc.
			 * So, update xmin and xmax. */
			TransactionId xmin = PG_UINT32_MAX;
			TransactionId xmax = 0;
			for (int i = 0; i < VCLUSTER_SEG_NUM_ENTRY; i++)
			{
				VLocator* vlocator = &seg_desc->locators[i];

				if (vlocator->xmin < xmin) {
					xmin = vlocator->xmin;
				}
				if (vlocator->xmax > xmax) {
					xmax = vlocator->xmax;
				}
			}
			seg_desc->xmin = xmin;
			/* Cutter may decide by xmax whether xmin/xmax of
			 * this segment desc is updated or not.
			 * So xmax must be updated after xmin. */
#ifdef HYU_LLT_STAT
			/* Set the time this segment has been full */
			INSTR_TIME_SET_CURRENT(seg_desc->filled_time);
#endif
			pg_memory_barrier();
			seg_desc->xmax = xmax;
		}
	}
	else if (alloc_seg_offset <= VCLUSTER_SEGSIZE &&
			 alloc_seg_offset + aligned_tuple_size > VCLUSTER_SEGSIZE)
	{
		/* Responsible for making a new segment for this cluster */
		
		/* TODO: If multiple process(foreground/background) prepare
		 * a new segment concurrently, need to add some arbitration code */
		dsap_new_seg = AllocNewSegment(cluster_type);

		/* Segment descriptor chain remains old to new */
		seg_desc->next = dsap_new_seg;
		pg_memory_barrier();
		vclusters->tail[cluster_type] = dsap_new_seg;
		pg_memory_barrier();

		/* Clear epoch */
		pg_memory_barrier();
		ClearTimestamp();
		goto retry;
	}
	else
	{
		/*
		 * Current segment is full, someone might(must) opening a
		 * new segmnet. Backoff and waiting it.
		 */
		pg_usleep(1000L);
		goto retry;
	}

	/* Clear epoch */
	pg_memory_barrier();
	ClearTimestamp();
}

/*
 * UpdateVersionStatistics
 *
 * Update the version statistics for classification
 */
void
VClusterUpdateVersionStatistics(TransactionId xmin, TransactionId xmax)
{
	uint64	len = xmax - xmin;

	/*
	 * Average value can be touched by multiple processes so that
	 * we need to decide only one process to update the value at a time.
	 */
	if (pg_atomic_test_set_flag(&vclusters->is_updating_stat))
	{
		if (unlikely(vclusters->average_ver_len == 0))
			vclusters->average_ver_len = len;
		else
			vclusters->average_ver_len =
					vclusters->average_ver_len * AVG_STAT_WEIGHT +
					len * (1.0 - AVG_STAT_WEIGHT);
		
		pg_atomic_clear_flag(&vclusters->is_updating_stat);
	}
}

/*
 * VClusterUpdateTransactionStatistics
 *
 * Update the transaction statistics (life cycle) for classification
 */
void
VClusterUpdateTransactionStatistics(FullTransactionId xid,
									FullTransactionId nextFullId)
{
	uint64	len = nextFullId.value - xid.value;
	
	if (!TransactionIdIsValid(xid.value))
		return;

	/*
	 * Average value can be touched by multiple processes so that
	 * we need to decide only one process to update the value at a time.
	 */
	if (pg_atomic_test_set_flag(&vclusters->is_updating_stat))
	{
		if (unlikely(vclusters->average_txn_len == 0))
			vclusters->average_txn_len = len;
		else
			vclusters->average_txn_len =
					vclusters->average_txn_len * AVG_STAT_WEIGHT +
					len * (1.0 - AVG_STAT_WEIGHT);

		pg_atomic_clear_flag(&vclusters->is_updating_stat);
	}
}

/*
 * VersionClassification
 *
 * Classify the version into each version cluster using version statistics
 */
static VCLUSTER_TYPE
VersionClassification(TransactionId xmin,
					  TransactionId xmax,
					  FullTransactionId nextFullId,
					  Snapshot snapshot)
{
	uint64_t		len;
	uint64_t		llt_boundary;
	TransactionId	recent_oldest_xid;

	/* LLT Classification */
	if (vclusters->average_txn_len * CLASSIFICATION_THRESHOLD_LLT >
				nextFullId.value)
		llt_boundary = 0;
	else
		llt_boundary = nextFullId.value -
				(vclusters->average_txn_len * CLASSIFICATION_THRESHOLD_LLT);

	recent_oldest_xid = 0;

	for (int i = 0; i < snapshot->xcnt; i++)
	{
		/* Find most recent transaction among llt in the snapshot */
		if (snapshot->xip[i] <= llt_boundary &&
			snapshot->xip[i] > recent_oldest_xid)
			recent_oldest_xid = snapshot->xip[i];
	}

	/*
	 * If the version is possibly visible by an LLT,
	 * classify it into LLT cluster.
	 */
	if (xmin <= recent_oldest_xid)
		return VCLUSTER_LLT;
	
	/* HOT/COLD Classification */
	len = xmax - xmin;
	if (len > vclusters->average_ver_len * CLASSIFICATION_THRESHOLD_COLD)
		return VCLUSTER_COLD;
	
	return VCLUSTER_HOT;
}

/*
 * AllocateNewSegmentInternal
 *
 * Internal implementation of AllocateNewSegment
 * Postmaster exceptionally call this function directly,
 * using its own dsa_area.
 */
static dsa_pointer
AllocNewSegmentInternal(dsa_area *dsa, VCLUSTER_TYPE cluster_type)
{
	dsa_pointer		ret;
	VSegmentDesc   *seg_desc;
	VSegmentId		new_seg_id;

	/* Allocate a new segment descriptor in shared memory */
	ret = dsa_allocate_extended(
			dsa, sizeof(VSegmentDesc), DSA_ALLOC_ZERO);
	
	//ereport(LOG, (errmsg("$$ sizeof(VSegmentDesc): %d", sizeof(VSegmentDesc))));

	/* Get a new segment id */
	new_seg_id = vclusters->reserved_seg_id[cluster_type];
	vclusters->reserved_seg_id[cluster_type] =
			pg_atomic_fetch_add_u32(&vclusters->next_seg_id, 1);

	/* Initialize the segment descriptor */
	seg_desc = (VSegmentDesc *)dsa_get_address(dsa, ret);
	seg_desc->dsap = ret;
	seg_desc->seg_id = new_seg_id;
	seg_desc->xmin = 0;
	seg_desc->xmax = 0;
	seg_desc->next = 0;
	seg_desc->timestamp = TS_NONE;
	seg_desc->counter = 0;
	pg_atomic_init_u32(&seg_desc->seg_offset, 0);

	/* Initialize the all vlocators in the segment index */
	for (int i = 0; i < VCLUSTER_SEG_NUM_ENTRY; i++)
	{
		/* Each vlocator contains its dsa_pointer itself */
		seg_desc->locators[i].dsap =
				ret + offsetof(VSegmentDesc, locators[i]);
	}

	/* Allocate a new segment file */
	VCacheCreateSegmentFile(new_seg_id);

	//ereport(LOG, (errmsg("@@ AllocNewSegmentInternal, seg: %d, type: %d",
	//			new_seg_id, cluster_type)));


	/* Returns dsa_pointer for the new VSegmentDesc */
	return ret;
}

/*
 * AllocateNewSegment
 *
 * Make a new segment file and its index
 * Returns the dsa_pointer of the new segment descriptor
 */
static dsa_pointer
AllocNewSegment(VCLUSTER_TYPE cluster_type)
{
	Assert(dsa_vcluster != NULL);

	return AllocNewSegmentInternal(dsa_vcluster, cluster_type);
}

/*
 * CutSegment
 *
 * Cut a segment of victim(vseg desc) and fix-up all.
 */
static void
CutSegment(VSegmentDesc* victim)
{
	for (int i = 0; i < VCLUSTER_SEG_NUM_ENTRY; i++) {
		VLocator*		vlocator;
		VLocatorFlag	flag;

		/* Start the consensus protocol for transaction(inserter). */

		/* 1) Get vlocator for cut. */
		vlocator = &victim->locators[i];

		/* 2) Let's play fetch-and-add to decide winner or loser. */
		flag = (VLocatorFlag) __sync_lock_test_and_set(&vlocator->flag, VL_DELETE);

		if (flag == VL_WINNER) {
			/* 3-1) Yes, i'm winner! Let's delete it(fix-up). */
			VChainFixUpOne(vlocator);
		}

		/* 3-2) Loser has not to do anything. */

		/* End the consensus protocol. */
	}
}

/*
 * CutVSegDesc
 *
 * Find cuttable vseg desc from vseg desc list and cut it(logical delete).
 */
static void
CutVSegDesc(VCLUSTER_TYPE cluster_type)
{
	dsa_pointer		dsap_victim;
	dsa_pointer		dsap_victim_prev;
	VSegmentDesc*	victim;
	VSegmentDesc*	victim_prev;
	dsa_pointer		dsap_old_tail;
	VSegmentDesc*	old_tail;
#ifdef HYU_LLT_STAT
	instr_time		curr_time;
	uint64			cuttime_us;
#endif

start_from_head:
	/* Find cuttable segment des */
	dsap_victim_prev = 0;
	victim_prev = NULL;
	dsap_victim = vclusters->head[cluster_type];
	victim = (VSegmentDesc *)dsa_get_address(
			dsa_vcluster, dsap_victim);

	for (;;) {
		if (victim->next == 0) {
			/* It is tail where new versions are inserted. */
			goto end;
		}
		
		if (victim->xmax == 0) {
			/* xmin/xmax is not updated yet. */
			/* We assume that updated xmax value never be 0. */
			goto next;
		}

		if (!SegIsInDeadZone(victim->xmin, victim->xmax)) {
			/* It is not dead. */
			goto next;
		}

#ifdef HYU_LLT_STAT
		/* Measures the time duration between the segment be full and cut */
		INSTR_TIME_SET_CURRENT(curr_time);

		/* curr_time -= victim->filled_time */
		INSTR_TIME_SUBTRACT(curr_time, victim->filled_time);

		cuttime_us = INSTR_TIME_GET_MICROSEC(curr_time);

		VStatisticUpdateCuttime(cluster_type, cuttime_us);

		//ereport(LOG, (errmsg("@@ CutVSegDesc, seg: %d, type: %d, xmin: %d, xmax: %d",
		//		victim->seg_id, cluster_type, victim->xmin, victim->xmax)));
#endif

		/* We can cut this segment. */
		CutSegment(victim);

		/* Detach it from vseg desc list. */
		if (dsap_victim_prev == 0) {
			/* Change the head because victim is head. */
			vclusters->head[cluster_type] = victim->next;
		}
		else {
			/* link prev to next. */
			victim_prev->next = victim->next;
		}
		
		/* Delete segmet file. */
		VCacheTryRemoveSegmentFile(victim->seg_id);

		/* Link it to the garbage list. */
		victim->next = 0;
		pg_memory_barrier();

		/* We assume that cutter process is only one. */
		/* First, update tail. */
		dsap_old_tail = vclusters->garbage_list_tail[cluster_type];
		old_tail = (VSegmentDesc *)dsa_get_address(
			dsa_vcluster, dsap_old_tail);
		vclusters->garbage_list_tail[cluster_type] = dsap_victim;
		pg_memory_barrier();

		/* Second, update next of old tail to victim. */
		old_tail->next = dsap_victim;

		/* OK. Set timestamp. Any vSorter
		 * after this timestamp never see this victim node. */
		pg_memory_barrier();
		victim->timestamp = GetCurrentTimestamp();
#ifdef HYU_LLT_STAT
		__sync_fetch_and_add(&vstatistic_desc->cnt_seg_logical_deleted, 1);
#endif

		if (dsap_victim_prev == 0) {
			/* Old head is deleted, so let's start from new head. */
			goto start_from_head;
		}

		/* Victim is deleted, so prev pointer is fix. */
		victim = victim_prev;
		dsap_victim = dsap_victim_prev;

next:
		/* Move to next node. */
		dsap_victim_prev = dsap_victim;
		victim_prev = victim;
		dsap_victim = victim->next;
		victim = (VSegmentDesc *)dsa_get_address(
				dsa_vcluster, dsap_victim);
	}

end:
	return;
}

/*
 * my_quick_die
 *
 * Callback function for process signal.
 * We manage only exit signal.
 */
void
my_quick_die(SIGNAL_ARGS)
{
	HOLD_INTERRUPTS();
	proc_exit(1);
}

/*
 * VClusterCutProcessMain
 *
 * Main function of vCutter process.
 */
static void
VClusterCutProcessMain(void)
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

	for (;;) {
		for (VCLUSTER_TYPE type = 0; type < VCLUSTER_NUM; type++) {
			CutVSegDesc(type);
		}
		sleep(0.0001);
	}
}

/*
 * GarbageCollect
 *
 * Get head of garbage list and free it if it is never accessible by others.
 * Accessiblility is known by checking timestamp.
 * If a node's timestamp < GetMinimumTimestamp(), it is safe to free.
 * We assume that garbage list is accessed by only ONE producer(cutter)
 * and only ONE consumer(garbage collector). producer access on tail and
 * consumer access on head.
 */
static void
GarbageCollect(VCLUSTER_TYPE cluster_type)
{
	dsa_pointer		dsap_victim;
	dsa_pointer		dsap_head;
	VSegmentDesc*	victim;
	VSegmentDesc*	head;
	TimestampTz		min_ts;

	/* Head is dummy node. */
	dsap_head = vclusters->garbage_list_head[cluster_type];
	head = (VSegmentDesc *)dsa_get_address(
			dsa_vcluster, dsap_head);

	dsap_victim = head->next;

	if (dsap_victim == 0) {
		/* No node. */
		goto end;
	}

	/* Get victim from head->next. */
	victim = (VSegmentDesc *)dsa_get_address(
			dsa_vcluster, dsap_victim);

	/* Get minimum timestamp in thread table. */
	min_ts = GetMinimumTimestamp();
	
	/* Check timestamp of vseg desc. */
	if (victim->timestamp >= min_ts) {
		/* This vseg desc is accessible by other process. It's not safe to free. */
		goto end;
	}

	/* Check timestamp of vlocators. */
	for (int i = 0; i < VCLUSTER_SEG_NUM_ENTRY; i++) {
		TimestampTz ts;
		ts = victim->locators[i].timestamp;
		if (ts >= min_ts || ts == TS_NONE) {
			/* This vlocator is accessible by other process. It's not safe to free. */
			goto end;
		}
	}

	/* OK. It's safe to free, but now we just make victim new head and free old head. */
	/* Update head. */
	/* Victim become new head. */
	vclusters->garbage_list_head[cluster_type] = dsap_victim;

	/* OK. Let's free old head. */
	dsa_free(dsa_vcluster, dsap_head);

#ifdef HYU_LLT_STAT
	__sync_fetch_and_add(&vstatistic_desc->cnt_seg_physical_deleted, 1);
#endif
end:
	return;
}

/*
 * GCProcessMain
 *
 * Main function of garbage collector process.
 */
static void
GCProcessMain(void)
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

	for (;;) {
		for (VCLUSTER_TYPE type = 0; type < VCLUSTER_NUM; type++) {
			GarbageCollect(type);
		}
		sleep(0.01);
	}
}

#endif /* HYU_LLT */
