#include <llb0seg.h>




/*
 seg_index_alloc(int num):
 	num : 	number of records' versions in the segment index

 @return : Segment Index

*/

SegNodePtr seg_index_alloc(int num_versions) {
	SegNodePtr seg_arr = (SegNodePtr) malloc (sizeof(SegNode) * num_versions);
	assert(seg_arr != NULL);

	for (int i = 0; i < num_versions; ++i) {
		seg_arr[i].v_start = seg_arr[i].v_loc = 0;
		seg_arr[i].prev_seg_ptr = seg_arr[i].next_seg_ptr = NULL;
		seg_arr[i].timestamp = 0;
		seg_arr[i].flag = VL_WINNER;
	}

	return (seg_arr);
}





