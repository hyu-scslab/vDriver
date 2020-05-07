#include <assert.h>


#include <llb0llb.h>


/*
 llb_sys_init(int num_versions, int table_size):
 	num_versions 		: the number of records' version in the segment index
	table_size 			: the number of records in the table ( number of records )
	thread_table_size	: size of thread table in llb layer.
						: Size is larger than the max number of
						: running threads concurrently.

@return (ret)

*/

LLB::LLB(int num_versions, int table_size, uint64_t thread_table_size):
	vdesc_sys(num_versions, thread_table_size)
{
	this->num_versions = num_versions;
	this->table_size = table_size;
	this->llb_status = LLB_LIVE;
}

LLB::~LLB() {

}


/*
 llb_insert(SegNodePtr ptr, VersionLoc v_loc, Xid xid, int seg_loc):
 	ptr 	: Segment node pointer from hash table in vDriver
	v_loc	: Versoin Locator including segment id and its offset
	xid 	: Transaction id
	seg_loc : location in segment index array
@return (ret)

*/
void LLB::insert(SegNodePtr ptr, VersionLoc v_loc, Xid xid_start, Xid xid_end, int seg_loc, int clu_no) {
	// Insert into vdesc list and segment index
	this->vdesc_sys.insert(v_loc, xid_start, xid_end, seg_loc, ptr, clu_no);
}

/*
 llb_lookup(SegNodePtr ptr, ThreadId tid, Xid xid):
 	ptr : Segment node pointer from hash table in vDriver
	tid : Thread id used in epoch-based algorithm
	xid : Transaction id
@return (version locator)

This returns version locator in segment index.

*/
VersionLoc LLB::lookup(SegNodePtr ptr, ThreadId tid, ReadView* read_view, dict_index_t* index) {
	VersionLoc v_loc = 0;
	
	// Get version locator

	v_loc = this->vdesc_sys.lookup(tid, read_view, ptr -> next_seg_ptr, ptr -> prev_seg_ptr, index);
	
	return v_loc;
}

/*
 llb_check(SegNodePtr ptr, ThreadId tid, dict_index_t* index):
 	ptr : Segment node pointer from hash table in vDriver
	tid : Thread id used in epoch-based algorithm
	index : index
@return true up to its removability

*/
int LLB::check(SegNodePtr ptr, ThreadId tid, dict_index_t* index, uint64_t *range, uint64_t len) {
	return (this->vdesc_sys.check(tid, ptr->next_seg_ptr, ptr->prev_seg_ptr, index, range, len));
}

/* 
 llb_cutter(Xid xid):
 	xid : Version number ( Transaction id ) 
@return (ret)

Current xid means that the version record whose version number is smaller than given thing,
** It gaurantees that " it is safe to cut ".

*/
void LLB::cut(uint64_t* range, uint64_t len) {
	this->vdesc_sys.cut(range, len);
}



void LLB::add_segment(Sid seg_no, int clu_no) {
	this->vdesc_sys.add_segment(seg_no, clu_no);
}
