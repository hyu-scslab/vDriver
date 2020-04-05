/*-------------------------------------------------------------------------
 *
 * vcache.h
 *	  hash table definitions for mapping VCacheTag to vcache indexes.
 *
 *
 *
 * src/include/storage/vcache.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef VCACHE_H
#define VCACHE_H

#include "port/atomics.h"
#include "storage/vcluster.h"

#define SEG_PAGESZ					(BLCKSZ)

typedef struct VCacheTag
{
	VSegmentId		seg_id;
	VSegmentPageId	page_id;
} VCacheTag;

typedef struct VCacheDesc
{
	/* ID of the cached page. seg_id 0 means this entry is unused */
	VCacheTag	tag;

	bool		is_dirty;		/* whether the page is not yet synced */

	/*
	 * Written bytes on this cache entry. If it gets same with the page size,
	 * it can be unpinned and ready to flush.
	 *
	 * NOTE: At this time, we don't care about tuples overlapped between
	 * two pages.
	 */ 
	pg_atomic_uint32	written_bytes;
	
	/*
	 * Cache entry with refcount > 0 cannot be evicted.
	 * We use refcount as a pin. The refcount of appending page
	 * should be kept above 1, and the last transaction which filled up
	 * the page should decrease it for unpinning it.
	 */
	pg_atomic_uint32	refcount;
} VCacheDesc;

#define VCACHEDESC_PAD_TO_SIZE	(SIZEOF_VOID_P == 8 ? 64 : 1)

typedef union VCacheDescPadded
{
	VCacheDesc	vcachedesc;
	char		pad[VCACHEDESC_PAD_TO_SIZE];
} VCacheDescPadded;

/* Metadata for vcache */
typedef struct VCacheMeta
{
	/*
	 * Indicate the cache entry which might be a victim for allocating
	 * a new page. Need to use fetch-and-add on this so that multiple
	 * transactions can allocate/evict cache entries concurrently.
	 */
	pg_atomic_uint64	eviction_rr_idx;

	/* Flag to sync about segmengt file io between vCutter and vSorter. */
	/* vCutter try to remove file of cutted-segment and
	 * vSorter can evict a page which is included in the cutted-segment.
	 * It's why this flag is needed. */
	/*
	 * 63          31          0  
	 * -------------------------
	 * |     A     |     B     |
	 * -------------------------
	 * A : 0 -> This segment is not cutted.
	 *          vSorter can evict or prune the page.
	 * A : 1 -> This segment is cutted.
	 * B : ref count.
	 * If one get the value (A : 1, B : 0), the one, whoever is
	 * vCutter or vSorter, must remove the file of segment.
	 */
	uint64_t			cutting_flag[VCLUSTER_MAX_SEGMENTS];
} VCacheMeta;

/* macros for VClusterDesc->cutting_flag */
#define CF_DEFAULT			(0x0000000000000000ULL) /* 64 bit */
#define CF_REF_MARK			(0x00000000FFFFFFFFULL)
#define CF_CUT_MARK			(0xFFFFFFFF00000000ULL)
#define CF_END_MARK			(0x0000000100000000ULL)
#define CF_IS_REF(flag) 	(flag & CF_REF_MARK)
#define CF_IS_CUT(flag)		(flag & CF_CUT_MARK)
#define CF_IS_END(flag) 	(!(flag ^ CF_END_MARK))


#define GetVCacheDescriptor(id) (&VCacheDescriptors[(id)].vcachedesc)
#define InvalidVCache			(INT_MAX)

/* in globals.c ... this duplicates miscadmin.h */
extern PGDLLIMPORT int NVCache;

extern int seg_fds[VCLUSTER_MAX_SEGMENTS];

extern Size VCacheShmemSize(void);
extern void VCacheInit(void);

extern void VCacheAppendTuple(VSegmentId seg_id,
							  VSegmentId reserved_seg_id,
							  VSegmentOffset seg_offset,
				  			  Size tuple_size,
				  			  const void *tuple,
                              VCLUSTER_TYPE cluster_type,
							  TransactionId xmin,
							  TransactionId xmax);
extern int VCacheReadTupleRef(VSegmentId seg_id,
							   VSegmentOffset seg_offset,
							   void **ret_tuple);
extern void VCacheReadTuple(VSegmentId seg_id,
							VSegmentOffset seg_offset,
							Size tuple_size,
							void *ret_tuple);

extern void VCacheCreateSegmentFile(VSegmentId seg_id);
extern void VCacheRemoveSegmentFile(VSegmentId seg_id);
extern void VCacheTryRemoveSegmentFile(VSegmentId seg_id);

extern bool VCacheIsValid(int cache_id);
void VCacheUnref(int cache_id);

#endif							/* VCACHE_H */
