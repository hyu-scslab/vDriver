/*-------------------------------------------------------------------------
 *
 * vcache_hash.h
 *	  hash table definitions for mapping VCacheTag to vcache indexes.
 *
 *
 *
 * src/include/storage/vcache_hash.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef VCACHE_HASH_H
#define VCACHE_HASH_H

/*
 * The shared vcache mapping hash table is partitioned to reduce contention.
 * To determine which partition lock a given tag requires, compute the tag's
 * hash code with VCacheHashCode(), then apply VCacheMappingPartitionLock().
 * NB: NUM_VCACHE_PARTITIONS must be a power of 2!
 * (I just brought these macros from buf_internals.h:127 - jongbin)
 */
#define VCacheHashPartition(hashcode) \
	((hashcode) % NUM_VCACHE_PARTITIONS)
#define VCacheMappingPartitionLock(hashcode) \
	(&MainLWLockArray[VCACHE_MAPPING_LWLOCK_OFFSET + \
		VCacheHashPartition(hashcode)].lock)

extern Size VCacheHashShmemSize(int size);
extern void VCacheHashInit(int size);
extern uint32 VCacheHashCode(const VCacheTag *tagPtr);

extern int VCacheHashLookup(const VCacheTag *tagPtr,
							uint32 hashcode);
extern int VCacheHashInsert(const VCacheTag *tagPtr,
							uint32 hashcode,
							int cache_id);
extern void VCacheHashDelete(const VCacheTag *tagPtr,
							 uint32 hashcode);

#endif							/* VCACHE_HASH_H */
