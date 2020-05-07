#ifndef __SEG_H__
#define __SEG_H__


#include "llb0include.h"

/* typedef struct & struct pointer */

typedef struct seg_node SegNode;
typedef struct seg_node* SegNodePtr;

/* End */

typedef enum {
	VL_WINNER,
	VL_APPEND,
	VL_DELETE,
} VLFlag;

/*
 Segment Index Node

 It contains those things

v_start 	: Version start
v_loc   	: Version locator
prev_ptr 	: previous segment element pointer ( previous record's version )
next_ptr 	: next segment element pointer	   ( next record's version )
timestamp	: time at logical deleted. 0 means it's not deleted.

flag		: Using fetch-and-add for contention between cutting ops and insert ops in tail node
*/

struct seg_node {

	Xid v_start;

	VersionLoc v_loc;

	SegNodePtr prev_seg_ptr;
	SegNodePtr next_seg_ptr;

	Timestamp timestamp;

	VLFlag flag;
};






SegNodePtr seg_index_alloc(int num_versions);






#endif /* __SEG_H__ */
