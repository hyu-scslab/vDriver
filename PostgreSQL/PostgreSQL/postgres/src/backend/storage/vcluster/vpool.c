/*-------------------------------------------------------------------------
 *
 * vpool.c
 *
 * Memory pool implementation for version cluster
 *
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/storage/vcluster/vpool.c
 *
 *-------------------------------------------------------------------------
 */
#ifdef HYU_LLT
#include "postgres.h"

/*
 * VPoolShmemSize
 *
 * compute the size of shared memory for a vpool
 */
Size
VPoolShmemSize(int size_pool, int bytes_entry)
{
	Size		size = 0;

	/* add the size of the VPool structure*/
	size = add_size(size, sizeof(VPool));

	/* add the size of the memory pool with the given size */
	size = add_size(size, (mul_size(size_pool, bytes_entry));

	return size;
}

/*
 * VPoolShmemInit
 *
 * Initialize vpool in shared memory with given size
 */
VPool*
VPoolShmemInit(char *pool_name, int size_pool, int bytes_entry)
{
	bool		foundPool,
				foundPoolIntr;
	VPool		*vpool;
	char		internal_pool_name[128];

	/* Initialize VPool structure */
	vpool = (VPool *)ShmemInitStruct(pool_name, sizeof(VPool), &foundPool);
	vpool->size_pool = size_pool;
	vpool->bytes_entry = bytes_entry;

	/* Initialize memory pool for VPool */
	sprintf(internal_pool_name, "%s_internal", pool_name);
	vpool->pool_ptr = ShmemInitStruct(internal_pool_name,
									  pool_size * bytes_entry,
									  &foundPoolIntr);

	/* Setup freelist of the pool */
	vpool->freelist = NULL;
	for (int i = 0; i < size_pool; i++) {
		char *entry = (vpool->pool_ptr + (i * bytes_entry);
		*((char**)entry) = vpool->freelist;
		vpool->freelist = entry;
	}

	return vpool;
}

#endif /* HYU_LLT */
