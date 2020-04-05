/*-------------------------------------------------------------------------
 *
 * cstatistic.c
 *
 * Statistics for vDriver & vanilla Implementation
 *
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/storage/vcluster/cstatistic.c
 *
 *-------------------------------------------------------------------------
 */
#ifdef HYU_COMMON_STAT
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


#include "storage/cstatistic.h"

/* Statistic info in shared memory */
CStatisticDesc*	cstatistic_desc;


uint64_t cnt_version_chain_vanilla;
uint64_t cnt_version_chain_vdriver;



/*
 * CStatisticShmemSize
 *
 * compute the size of shared memory
 */
Size
CStatisticShmemSize(void)
{
	Size		size = 0;

	size = add_size(size, sizeof(CStatisticDesc));

	return size;
}

/*
 * CStatisticInit
 *
 * Initialize shared data structures related to statistics in shared memory
 */
void
CStatisticInit(void)
{
	bool		foundDesc;

	cstatistic_desc = (CStatisticDesc*)
		ShmemInitStruct("vDriver and vanilla Statistics Descriptor",
						sizeof(CStatisticDesc), &foundDesc);

	/* Initialize statistics */
	cstatistic_desc->cnt_chain = 0;
}


#endif /* HYU_COMMON_STAT */
