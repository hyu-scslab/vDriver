/*-------------------------------------------------------------------------
 *
 * vstatistic.c
 *
 * Statistics for vDriver Implementation
 *
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/storage/vcluster/vstatistic.c
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

#include "storage/vstatistic.h"

/* thread table in shared memory*/
VStatisticDesc*	vstatistic_desc;


/* local functions */
static void MonitorProcessMain(void);
//static void PrintStatistics(void);



/*
 * VStatisticShmemSize
 *
 * compute the size of shared memory
 */
Size
VStatisticShmemSize(void)
{
	Size		size = 0;

	size = add_size(size, sizeof(VStatisticDesc));

	return size;
}

/*
 * VStatisticInit
 *
 * Initialize shared data structures related to statistics in shared memory
 */
void
VStatisticInit(void)
{
	bool		foundDesc;

	vstatistic_desc = (VStatisticDesc*)
		ShmemInitStruct("vDriver Statistics Descriptor",
						sizeof(VStatisticDesc), &foundDesc);

	/* Initialize statistics */
	vstatistic_desc->cnt_inserted = 0;
	vstatistic_desc->cnt_first_prune = 0;
	vstatistic_desc->cnt_after_first_prune = 0;
	vstatistic_desc->cnt_page_evicted = 0;
	vstatistic_desc->cnt_page_second_prune = 0;

	vstatistic_desc->cnt_logical_deleted = 0;
	vstatistic_desc->cnt_seg_logical_deleted = 0;
	vstatistic_desc->cnt_seg_physical_deleted = 0;

    for (int i = 0; i < VCLUSTER_NUM; i++) {
        vstatistic_desc->cnt_inserted_cluster[i] = 0;
        vstatistic_desc->cnt_first_prune_cluster[i] = 0;
        vstatistic_desc->cnt_after_first_prune_cluster[i] = 0;
        vstatistic_desc->cnt_page_evicted_cluster[i] = 0;
        vstatistic_desc->cnt_page_second_prune_cluster[i] = 0;
    }
	
	memset(vstatistic_desc->bucket_cuttime,
			0, sizeof(uint32) * NUM_CUTTIME_BUCKET * VCLUSTER_NUM);
}

/*
 * StartMonitor
 *
 * Fork monitor process.
 */
pid_t
StartMonitor(void)
{
	pid_t		monitor_pid;

	/* Start a monitor process. */
	monitor_pid = fork_process();
	if (monitor_pid == -1) {
		/* error */
	}
	else if (monitor_pid == 0) {
		/* child */
		MonitorProcessMain(); /* no return */
	}
	else {
		/* parent */
	}

	return monitor_pid;
}

/*
 * UpdateCuttimeBucket
 *
 * Insert a new cuttime into the corresponding cuttime bucket
 */
void
VStatisticUpdateCuttime(VCLUSTER_TYPE cluster_type, uint64 cuttime_us)
{
	int bucket_idx;

	bucket_idx = cuttime_us / CUTTIME_BUCKET_UNIT;
	if (bucket_idx >= NUM_CUTTIME_BUCKET)
		bucket_idx = NUM_CUTTIME_BUCKET - 1;

	/*
	 * Only the cutter process calls this function so that
	 * we don't need any consensus here.
	 */
	vstatistic_desc->bucket_cuttime[cluster_type][bucket_idx]++;
}

/*
 * PrintStatistics
 *
 * Print statistics.
 * legacy code.
 */
//static void
//PrintStatistics(void)
//{
//	int64_t cnt_inserted;
//	int64_t cnt_first_prune;
//	int64_t cnt_after_first_prune;
//	double percent_first_prune;
//
//	int64_t prev_cnt_inserted = 0;
//	int64_t prev_cnt_first_prune = 0;
//
//	int64_t cnt_page_evicted;
//	int64_t cnt_page_second_prune;
//	double percent_second_prune;
//
//	int64_t cnt_seg_logical_deleted;
//	int64_t cnt_seg_physical_deleted;
//	int64_t cnt_logical_deleted;
//	int64_t delta_deleted_inserted;
//
//	for (;;) {
//
//		cnt_inserted = vstatistic_desc->cnt_inserted;
//		cnt_first_prune	= vstatistic_desc->cnt_first_prune;
//		cnt_after_first_prune = vstatistic_desc->cnt_after_first_prune;
//		percent_first_prune = (double) (cnt_first_prune - prev_cnt_first_prune) / 
//			(cnt_inserted - prev_cnt_inserted) * 100;
//
//		prev_cnt_inserted = cnt_inserted;
//		prev_cnt_first_prune = cnt_first_prune;
//
//		cnt_page_evicted = vstatistic_desc->cnt_page_evicted;
//		cnt_page_second_prune = vstatistic_desc->cnt_page_second_prune;
//		percent_second_prune = (double) cnt_page_second_prune / cnt_page_evicted * 100;
//
//		cnt_seg_logical_deleted = vstatistic_desc->cnt_seg_logical_deleted;
//		cnt_seg_physical_deleted = vstatistic_desc->cnt_seg_physical_deleted;
//		cnt_logical_deleted = vstatistic_desc->cnt_logical_deleted;
//		delta_deleted_inserted = cnt_inserted - cnt_logical_deleted;
//
//
//		elog(WARNING, "HYU_LLT : statistic ============================\n"
//				"HYU_LLT\n"
//				"HYU_LLT\n"
//				"HYU_LLT         insert rec           :  %8ld\n"
//				"HYU_LLT         1st_prune            :  %8ld  (%3.1lf%%)\n"
//				"HYU_LLT         after first prune    :  %8ld\n"
//				"HYU_LLT         evicted page         :  %8ld\n"
//				"HYU_LLT         2nd_prune            :  %8ld  (%3.1lf%%)\n"
//				"HYU_LLT         logical delete seg   :  %8ld\n"
//				"HYU_LLT         logical delete rec   :  %8ld\n"
//				"HYU_LLT         physical delete seg  :  %8ld\n"
//				"HYU_LLT         physical delete rec  :  %8ld\n"
//				"HYU_LLT         delta ins - del rec  :  %8ld\n"
//				"HYU_LLT\n"
//				"HYU_LLT\n",
//				cnt_inserted,
//				cnt_first_prune, percent_first_prune,
//				cnt_after_first_prune,
//				cnt_page_evicted,
//				cnt_page_second_prune, percent_second_prune,
//				cnt_seg_logical_deleted,
//				cnt_seg_logical_deleted * VCLUSTER_SEG_NUM_ENTRY,
//				cnt_seg_physical_deleted,
//				cnt_seg_physical_deleted * VCLUSTER_SEG_NUM_ENTRY,
//				delta_deleted_inserted
//			);
//
//		sleep(1);
//	}
//}

/*
 * MonitorProcessMain
 *
 * Main function of monitor process.
 * TODO: We don't need monitor process any more.
 * llt_get_stat() function take this place.
 */
static void
MonitorProcessMain(void)
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
		//PrintStatistics();
		sleep(1);
	}
}

#endif /* HYU_LLT */
