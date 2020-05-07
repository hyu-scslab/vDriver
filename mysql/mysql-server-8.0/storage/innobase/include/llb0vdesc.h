#ifndef llb0vdesc_h
#define llb0vdesc_h

#include "llb0seg.h"
#include "llb0time.h"

#include "read0types.h"

#include <vector>
struct thread_table_node {
	Timestamp timestamp;
};

typedef struct thread_table_node ThreadTableNode;
typedef vector<ThreadTableNode> ThreadTable;

/* 
 We define VDesc structure as vdesc_node list.

 Version Descriptor system
 It contains those things

head_vdesc_ptr  : Head vdesc node pointer 
tail_vdesc_ptr  : Tail vdesc node pointer

used_seg_num    : Number of segment fully filled
max_seg_id      : Segment id not used for the first

num_versions    : number of records' versions in the segment index


cut_seg         : This value is for counting reclaimed segment index " FOR TESTING "

vdesc_ptr_arr[] : Array for utilization

*/
class vdesc_node {
	public:
	vdesc_node();
	vdesc_node(Sid, int);
	~vdesc_node();

	Sid seg_id;
	Xid v_min;
	Xid v_max;
	
	int cnt;
	uint32_t written_bytes;

	bool full;
	
	SegNodePtr seg_ptr;
	vdesc_node* next_vdesc_ptr;
};


typedef vdesc_node	VDescNode;
typedef vdesc_node*	VDescNodePtr;


class VDesc {
	
	private:
	
		typedef enum {VDESC_LIVE, VDESC_EXIT} VDescStatus;

		// Version Descriptor list head & tail pointer
		VDescNodePtr head_vdesc_ptr;	
		VDescNodePtr tail_vdesc_ptr;

		// Version Descriptor list head & tail pointer used in garbage collector
		VDescNodePtr garbage_list_head;
		VDescNodePtr garbage_list_tail;

		// Garbage collector thread
		thread garbage_collector;

		// Number of versions in segment index
		int num_versions;

		// Vdesc status
		volatile VDescStatus vdesc_status;

		void vdesc_insert_low(VDescNodePtr, VersionLoc, int, Xid, SegNodePtr);

		void vdesc_cutter_segment(SegNodePtr, bool);
		void vdesc_cutter_internal(VDescNodePtr);

		Timestamp get_timestamp_min();
		void garbage_collector_func();

		void remove_file_if_possible(uint32_t);

		uint64_t num_seg_cut_hi;
		uint64_t num_seg_cut_lo;
		uint64_t num_file_cut;

	public:

		VDesc(int);
		~VDesc();

		
		void insert(VersionLoc, Xid, Xid, int, SegNodePtr);

		void cut(uint64_t*, uint64_t);
		void add_segment(Sid);

		VDescNodePtr get_tail_ptr() { return (VDescNodePtr)__sync_fetch_and_add(&(this->tail_vdesc_ptr), 0); }

		/* Counting some variables */
		uint64_t total_seg_cut_hi() { return (this->num_seg_cut_hi); }
		uint64_t total_seg_cut_lo() { return (this->num_seg_cut_lo); }
		uint64_t total_file_cut() { return (this->num_file_cut); }

};
/*
   Lookup, thread table(global), vdesc_ptr_arr(global).
API : lookup, cut, insert, add_segment . ALL thing need
*/
class VDesc_sys {
	public:
		VDesc_sys(){};
		VDesc_sys(int, uint64_t);
		~VDesc_sys();

		void insert(VersionLoc, Xid, Xid, int, SegNodePtr, int);
		VersionLoc lookup(Tid, ReadView*, SegNodePtr, SegNodePtr, dict_index_t*);
    int check(Tid, SegNodePtr, SegNodePtr, dict_index_t*, uint64_t*, uint64_t);
		void cut(uint64_t*, uint64_t);
		void add_segment(Sid, int);

		VDescNodePtr get_tail_ptr(int clu_no) { return (vdesc[clu_no]->get_tail_ptr()); }

		/* For statistics */
		uint64_t total_seg_cut_hi();
		uint64_t total_seg_cut_lo();
		uint64_t total_file_cut();

	private:
		VDesc* vdesc[3];
		
};


bool is_it_dead(Xid, Xid, uint64_t*, uint64_t);
#endif /* llb0vdesc_h */

