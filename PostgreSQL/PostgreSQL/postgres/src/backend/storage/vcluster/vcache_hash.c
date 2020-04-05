/*-------------------------------------------------------------------------
 *
 * vcache_hash.c
 *
 * hash table implementation for mapping VCacheTag to vcache indexes
 *
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/storage/vcluster/vcache_hash.c
 *
 *-------------------------------------------------------------------------
 */
#ifdef HYU_LLT
#include "postgres.h"

#include "storage/lwlock.h"
#include "storage/shmem.h"
#include "storage/vcache.h"
#include "storage/vcache_hash.h"
#include "utils/hsearch.h"

/* entry for vcache lookup hashtable */
typedef struct
{
	VCacheTag	key;			/* Tag of a disk page */
	int			id;				/* Associated cache ID */
} VCacheLookupEnt;

static HTAB *SharedVCacheHash;

/*
 * VCacheHashShmemSize
 *
 * compute the size of shared memory for vcache_hash
 */
Size
VCacheHashShmemSize(int size)
{
	return hash_estimate_size(size, sizeof(VCacheLookupEnt));
}

/*
 * VCacheHashInit
 *
 * Initialize vcache_hash in shared memory
 */
void
VCacheHashInit(int size)
{
	HASHCTL		info;

	/* VCacheTag maps to VCache */
	info.keysize = sizeof(VCacheTag);
	info.entrysize = sizeof(VCacheLookupEnt);
	info.num_partitions = NUM_VCACHE_PARTITIONS;

	SharedVCacheHash = ShmemInitHash("Shared VCache Lookup Table",
								     size, size,
								     &info,
								     HASH_ELEM | HASH_BLOBS | HASH_PARTITION);
}

/*
 * VCacheHashCode
 *
 * Compute the hash code associated with a VCacheTag
 * This must be passed to the lookup/insert/delete routines along with the
 * tag.  We do it like this because the callers need to know the hash code
 * in order to determine which buffer partition to lock, and we don't want
 * to do the hash computation twice (hash_any is a bit slow).
 */
uint32
VCacheHashCode(const VCacheTag *tagPtr)
{
	return get_hash_value(SharedVCacheHash, (void *) tagPtr);
}

/*
 * VCacheHashLookup
 *
 * Lookup the given VCacheTag; return vcache ID, or -1 if not found
 * Caller must hold at least share lock on VCacheMappingLock for tag's partition
 */
int
VCacheHashLookup(const VCacheTag *tagPtr, uint32 hashcode)
{
	VCacheLookupEnt *result;

	result = (VCacheLookupEnt *)
			hash_search_with_hash_value(SharedVCacheHash,
										(void *) tagPtr,
										hashcode,
										HASH_FIND,
										NULL);
	if (!result)
		return -1;

	return result->id;
}

/*
 * VCacheHashInsert
 *
 * Insert a hashtable entry for given tag and cache ID,
 * unless an entry already exists for that tag
 *
 * Returns -1 on successful insertion. If a conflicting entry exists
 * already, returns the cache ID in that entry.
 *
 * Caller must hold exclusive lock on VCacheMappingLock for tag's partition
 */
int
VCacheHashInsert(const VCacheTag *tagPtr, uint32 hashcode, int cache_id)
{
	VCacheLookupEnt    *result;
	bool				found;
	
	Assert(cache_id >= 0);		/* -1 is reserved for not-in-table */
	
	result = (VCacheLookupEnt *)
			hash_search_with_hash_value(SharedVCacheHash,
										(void *) tagPtr,
										hashcode,
										HASH_ENTER,
										&found);

	if (found)					/* found something already in the table */
		return result->id;
	
	result->id = cache_id;
	return -1;
}

/*
 * VCacheHashDelete
 *
 * Delete the hashtable entry for given tag (which must exist)
 *
 * Caller must hold exclusive lock on VCacheMappingLock for tag's partition
 */
void
VCacheHashDelete(const VCacheTag *tagPtr, uint32 hashcode)
{
	VCacheLookupEnt *result;
	
	result = (VCacheLookupEnt *)
			hash_search_with_hash_value(SharedVCacheHash,
										(void *) tagPtr,
										hashcode,
										HASH_REMOVE,
										NULL);
	
	if (!result)				/* shouldn't happen */
		elog(ERROR, "shared vcache hash table corrupted");
}

#endif /* HYU_LLT */
