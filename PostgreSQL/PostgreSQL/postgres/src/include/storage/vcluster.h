/*-------------------------------------------------------------------------
 *
 * vcluster.h
 *	  version cluster
 *
 *
 *
 * src/include/storage/vcluster.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef VCLUSTER_H
#define VCLUSTER_H

#include "c.h"
#include "portability/instr_time.h"
#include "utils/dsa.h"
#include "utils/snapshot.h"
#include "utils/snapmgr.h"
#include "utils/dynahash.h"
#include "utils/timestamp.h"

enum {
	VCLUSTER_HOT,
	VCLUSTER_COLD,
	VCLUSTER_LLT,
	VCLUSTER_NUM = 3
};
typedef uint32_t VCLUSTER_TYPE;

/* Size of a segment of a version cluster */
#define VCLUSTER_SEGSIZE    (64*1024)

/* Version tuple size */
#ifndef VCLUSTER_TUPLE_SIZE /* configurable outside */
#define VCLUSTER_TUPLE_SIZE	(256)
#endif

#define VCLUSTER_TUPLE_LEN	(VCLUSTER_TUPLE_SIZE - 12)
/* Layout of one record on VSegment. Size must be VCLUSTER_TUPLE_SIZE. */
/* These are used to second-prune. */
typedef struct {
	char tuple[VCLUSTER_TUPLE_LEN];
    VCLUSTER_TYPE cluster_type;
	TransactionId xmin;
	TransactionId xmax;
} VRecord;

/* Number of tuple entry in a vsegment */
#define VCLUSTER_SEG_NUM_ENTRY	((VCLUSTER_SEGSIZE) / (VCLUSTER_TUPLE_SIZE))

/* Max number of segments we can manage. */
//#define VCLUSTER_MAX_SEGMENTS		10000
#define VCLUSTER_MAX_SEGMENTS		100000

typedef uint32_t VSegmentId;
typedef uint32_t VSegmentOffset;
typedef uint32_t VSegmentPageId;

typedef int64_t	PrimaryKey;

/* Flag to decide winner or loser between transaction and cutter. */
typedef enum
{
	VL_WINNER,			/* Winner's flag */
	VL_APPEND,
	VL_DELETE,
} VLocatorFlag;

typedef struct {
	dsa_pointer			dsap;

	TransactionId		xmin;
	TransactionId		xmax;
	VSegmentId			seg_id;
	VSegmentOffset		seg_offset;

	VLocatorFlag		flag;
	TimestampTz			timestamp;

	/*
	 * Version chain pointer for a single tuple.
	 * need to be converted from dsa_pointer to (VLocator *)
	 */
	dsa_pointer			dsap_prev;
	dsa_pointer			dsap_next;
} VLocator;

typedef struct {
	/* dsa pointer of this object */
	dsa_pointer			dsap;

	VSegmentId			seg_id;
	TransactionId		xmin;
	TransactionId		xmax;
	
	/* Need to be converted from dsa_pointer to (VSegmentDesc *) */
	/* vseg desc linked list is accessed by only cutter. */
	dsa_pointer			next;

	/* Timestamp : we can check whether this node is visible
	 * for transaction processes or not. */
	TimestampTz			timestamp;

	/* Counter about how many versions written in this segment.
	 * Last precess has to update xmin & xmax of this segment desc. */
	int64_t				counter;

	/* Segment offset where the next version tuple will be appended.
	 * Backend process will use fetch-and-add instruction on this variable
	 * to allocate the offset, so the value can be higher than the segment
	 * size. In this case, one of the backend process who get this high value
	 * should open a new segment. */
	pg_atomic_uint32	seg_offset;

	/* Just embed segment index into here  */
	VLocator			locators[VCLUSTER_SEG_NUM_ENTRY];

#ifdef HYU_LLT_STAT
	/* The time this segment has been full */
	instr_time			filled_time;
#endif

} VSegmentDesc;

typedef struct {
	/* need to be converted from dsa_pointer to (VSegmentDesc *) */
	dsa_pointer			head[VCLUSTER_NUM];

	/* need to be converted from dsa_pointer to (VSegmentDesc *) */
	dsa_pointer			tail[VCLUSTER_NUM];

	/* need to be converted from dsa_pointer to (VSegmentDesc *) */
	/* garbage list is accessed by ONE producer(cutter) and
	 * ONE consumer(garbage collector), and this is implement
	 * that a producer and a consumer can access it concurrently. */
	/* There is one dummy node. */
	dsa_pointer			garbage_list_head[VCLUSTER_NUM];
	dsa_pointer			garbage_list_tail[VCLUSTER_NUM];

	/* next segment id for allocation */
	pg_atomic_uint32	next_seg_id;

	/* reserved segment id for each cluster */
	VSegmentId			reserved_seg_id[VCLUSTER_NUM];

	/* Padding for frequently updated variables */
	char				pad1[PG_CACHE_LINE_SIZE];
	
	/* to make consensus for multiple transactions to update the stats */
	pg_atomic_flag		is_updating_stat;
	
	/* Padding for frequently updated variables */
	char				pad2[PG_CACHE_LINE_SIZE];

	/* stat value for HOT/COLD classification */
	double				average_ver_len;

	/* stat value for LLT classification */
	double				average_txn_len;
	
} VClusterDesc;

extern VClusterDesc	*vclusters;
extern dsa_area		*dsa_vcluster;

extern Size VClusterShmemSize(void);
extern void VClusterShmemInit(void);
extern void VClusterDsaInit(void);
extern void VClusterAttachDsa(void);
extern void VClusterDetachDsa(void);
extern pid_t StartVCutter(void);
extern pid_t StartGC(void);

void VClusterUpdateVersionStatistics(TransactionId xmin, TransactionId xmax);
void VClusterUpdateTransactionStatistics(FullTransactionId xid,
										 FullTransactionId nextFullId);

#if 0
extern bool VClusterLookupTuple(Oid rel_node,
								PrimaryKey primary_key,
								Size size,
								Snapshot snapshot,
								void *ret_tuple);
#endif
extern int VClusterLookupTuple(Oid rel_node,
							   PrimaryKey primary_key,
							   Snapshot snapshot,
							   void **ret_tuple);

extern void VClusterAppendTuple(Oid rel_node,
								PrimaryKey primary_key,
								TransactionId xmin,
								TransactionId xmax,
								Snapshot snapshot,
								Size tuple_size,
								const void *tuple);


extern void my_quick_die(SIGNAL_ARGS);

#endif							/* VCLUSTER_H */
