/*-------------------------------------------------------------------------
 *
 * vchain_hash.h
 *	  hash table definitions for mapping primary key to vchain.
 *
 *
 *
 * src/include/storage/vchain_hash.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef VCHAIN_HASH_H
#define VCHAIN_HASH_H

#include "storage/vcluster.h"

typedef struct VChainTag
{
	PrimaryKey		primary_key;
	Oid				rel_node;
} VChainTag;

/* in globals.c ... this duplicates miscadmin.h */
extern PGDLLIMPORT int NVChainExpected;

/*
 * The shared vchain mapping hash table is partitioned to reduce contention.
 * To determine which partition lock a given tag requires, compute the tag's
 * hash code with VChainHashCode(), then apply VChainMappingPartitionLock().
 * NB: NUM_VCHAIN_PARTITIONS must be a power of 2!
 * (I just brought these macros from buf_internals.h:127 - jongbin)
 */
#define VChainHashPartition(hashcode) \
	((hashcode) % NUM_VCHAIN_PARTITIONS)
#define VChainMappingPartitionLock(hashcode) \
	(&MainLWLockArray[VCHAIN_MAPPING_LWLOCK_OFFSET + \
		VChainHashPartition(hashcode)].lock)

extern Size VChainHashShmemSize(int size);
extern void VChainHashInit(int size);
extern uint32 VChainHashCode(const VChainTag *tagPtr);

extern bool VChainHashLookup(const VChainTag *tagPtr,
							 uint32 hashcode,
							 dsa_pointer *ret);
extern bool VChainHashInsert(const VChainTag *tagPtr,
							 uint32 hashcode,
							 dsa_pointer *ret);
extern void VChainHashDelete(const VChainTag *tagPtr,
							 uint32 hashcode);

#endif							/* VCHAIN_HASH_H */
