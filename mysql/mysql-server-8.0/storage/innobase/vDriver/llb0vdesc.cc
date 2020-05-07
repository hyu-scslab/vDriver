#include "llb0vdesc.h"

/* segment file descriptor */
int seg_fd[NUM_SEG];

/* segment flag map */
int seg_flag[NUM_SEG];

/* vdesc pointer array */
VDescNodePtr vdesc_ptr_arr[NUM_SEG];

/* Thread table to manage timestamp for epoch-based things */
ThreadTable thread_table;

/* vdesc_node class */
vdesc_node::vdesc_node() {
	this->seg_id = 0;
	this->v_min = 0;
	this->v_max = 0;
	this->full = false;
	this->seg_ptr = nullptr;
	this->next_vdesc_ptr = nullptr;
	this->cnt = 0;
}

vdesc_node::vdesc_node (Sid seg_id, int num_versions) {
	this->seg_id = seg_id;
	this->v_min = UINT64_MAX;
	this->v_max = 0;
	this->written_bytes = 0;
	this->full = false;
	this->seg_ptr = seg_index_alloc(num_versions);
	this->next_vdesc_ptr = nullptr;
	this->cnt = 0;
}

vdesc_node::~vdesc_node() {
	if (this->seg_ptr != nullptr)
		free(this->seg_ptr);
}

/* Done */

VDesc::VDesc(int num_versions){

	this->num_versions = num_versions;
	this->num_seg_cut_hi = this->num_seg_cut_lo = this->num_file_cut = 0;	

	VDescNodePtr ptr = new VDescNode();
	this->head_vdesc_ptr = ptr;
	this->tail_vdesc_ptr = ptr;

	ptr = new VDescNode();
	this->garbage_list_head = ptr;
	this->garbage_list_tail = ptr;

	this->vdesc_status = VDESC_LIVE;

	garbage_collector = thread([&](){garbage_collector_func();});
}

VDesc::~VDesc(){

	this->vdesc_status = VDESC_EXIT;

	VDescNodePtr tmp = this->head_vdesc_ptr->next_vdesc_ptr;
	VDescNodePtr cur = this->head_vdesc_ptr;

	delete (cur);

	cur = tmp;
	while (cur && cur != this->tail_vdesc_ptr) {
		tmp = cur->next_vdesc_ptr;
		delete (cur);
		cur = tmp;
	}
	
	if (cur)
		delete (cur);
	
	this->garbage_collector.join();

	// TODO: free garbage list
	free(this->garbage_list_head);

}


/*
 vdesc_insert_low(VDescNodePtr vdesc_ptr, VersionLoc v_loc, int seg_loc, Xid xid, SegNodePtr tail_ptr):
	vdesc_ptr 	: Current vDescriptor node pointer
	v_loc    	: Version locator
	seg_loc     : Location in the segment index
	xid         : Version start
	tail_ptr    : Current record tail pointer

*/

void VDesc::vdesc_insert_low(VDescNodePtr vdesc_ptr, VersionLoc v_loc, int seg_loc, Xid xid, SegNodePtr tail_ptr) {
	SegNodePtr seg_ptr = vdesc_ptr->seg_ptr;
	SegNodePtr tmp_ptr = nullptr;
	SegNodePtr prev_ptr = nullptr;

	seg_ptr[seg_loc].v_start = xid;
	seg_ptr[seg_loc].v_loc = v_loc;

	seg_ptr[seg_loc].next_seg_ptr = tail_ptr;

	VLFlag flag;
	VLFlag vic_flag;
	
	while (true) {
		// 1. Get tail's previous pointer.
		// 	  tail_ptr is dummy node.
		tmp_ptr = tail_ptr -> prev_seg_ptr;

		// 2. Fetch-and-add its flag value
		flag = __sync_lock_test_and_set(&tmp_ptr->flag, VL_APPEND);

		// 3. If ret != 0, "update worker" should wait for changing its previous pointer by "cutter", then retry same routine.
		//    else, "update worker" is winner !!! Keep going its routine.
		if (flag == VL_DELETE) {
			while (true) {
				if (tmp_ptr != (SegNodePtr)__sync_fetch_and_add(&tail_ptr->prev_seg_ptr, 0)) {
					break;
				}
			}
		} else
			break;
	}

	seg_ptr[seg_loc].prev_seg_ptr = tmp_ptr;

	tail_ptr->prev_seg_ptr = tmp_ptr->next_seg_ptr = &seg_ptr[seg_loc];

	if (__sync_fetch_and_add(&vdesc_ptr->cnt, 1) == this->num_versions - 1) {
		vdesc_ptr->full = true;
	}

	flag = __sync_lock_test_and_set(&tmp_ptr->flag, VL_WINNER);
	
	if (flag == VL_DELETE) {
		
		while(true) {
			
			prev_ptr = tmp_ptr->prev_seg_ptr;
			
			vic_flag = __sync_lock_test_and_set(&prev_ptr->flag, VL_APPEND);

			if (vic_flag == VL_DELETE) {
				while (prev_ptr == __sync_fetch_and_add(&tmp_ptr->prev_seg_ptr, 0)) {};
				continue;
			}

			vdesc_cutter_segment(tmp_ptr, F_INDIRECT);

			vic_flag = __sync_lock_test_and_set(&prev_ptr->flag, VL_WINNER);

			if (vic_flag == VL_DELETE) {
				tmp_ptr = prev_ptr;
				continue;
			}

			break;
		}
	}

}


/*
 vdesc_insert(VersionLoc v_loc, Xid xid, int seg_loc, SegNodePtr tail_seg_ptr):
    v_loc       : Version locator
    xid         : Version start
    seg_loc     : Location in the segment index
    tail_ptr    : Current tail segment pointer 

@return (ret)
   
*/

void VDesc::insert(VersionLoc v_loc, Xid xid_start, Xid xid_end, int seg_loc, SegNodePtr tail_seg_ptr) {

	// You have to chan
	VDescNodePtr vdesc_ptr = vdesc_ptr_arr[SEG_ID(v_loc)];


	Xid v = vdesc_ptr->v_min;
	
	while (xid_start < v &&
			!__sync_bool_compare_and_swap(&(vdesc_ptr->v_min), v, xid_start))
		v = vdesc_ptr->v_min;

    v = vdesc_ptr->v_max;

	while (xid_end > v &&
			!__sync_bool_compare_and_swap(&(vdesc_ptr->v_max), v, xid_end))
		v = vdesc_ptr->v_max;

	vdesc_insert_low(vdesc_ptr, v_loc, seg_loc, xid_start, tail_seg_ptr);

}


/*
 vdesc_cutter_segment(SegNodePtr seg_ptr, bool mode):
    seg_ptr  :  segment index pointer to be cut (reclaimable segment index)
    mode     :  if true, Fix-up op is executed by "Cutter"
                else, Fix-up op is delegated to "Updater"

@return (void)

There are two mode for vdesc_cutter_segment

F_DIRECT ( for cutter )
F_INDIRECT ( for update worker )

*/

void VDesc::vdesc_cutter_segment(SegNodePtr seg_ptr, bool mode) {
	assert(seg_ptr != NULL);
	VLFlag flag;

	if (mode == F_DIRECT) {

		for (int i = 0; i < this->num_versions; ++i) {

			// 1. Fetch-and-add its flag value
			flag = __sync_lock_test_and_set(&seg_ptr[i].flag, VL_DELETE);

			// 2. If return value != 0, delegate this work to "update worker"
			//    Else, Do fix-up itself
			if (flag != VL_WINNER)

				continue;

			else {
				// We should guarantee this step using by "Memory Barrier"

				// A. Change its previous pointer's next pointer
				seg_ptr[i].prev_seg_ptr -> next_seg_ptr = seg_ptr[i].next_seg_ptr;
				MEMORY_BARRIER;

				// B. Change its next pointer's previous pointer
				seg_ptr[i].next_seg_ptr -> prev_seg_ptr = seg_ptr[i].prev_seg_ptr;
				MEMORY_BARRIER;

				// C. Then get its timestamp
				seg_ptr[i].timestamp = get_timestamp();
			}
		}

	} else if (mode == F_INDIRECT) {

		// Same step as above one
		seg_ptr -> prev_seg_ptr -> next_seg_ptr = seg_ptr -> next_seg_ptr;
		MEMORY_BARRIER;

		seg_ptr -> next_seg_ptr -> prev_seg_ptr = seg_ptr -> prev_seg_ptr;
		MEMORY_BARRIER;

		seg_ptr->timestamp = get_timestamp();

	} else
		assert(false);
}


/*
   vdesc_cutter_internal(VDescNodePtr cut_vdesc_ptr, Xid xid):
cut_vdesc_ptr   : Version descriptor pointer to be cut (reclaimable VDesc)
xid             : Reclaimable maximum version

@return (void)

 */
void VDesc::vdesc_cutter_internal(VDescNodePtr cut_vdesc_ptr) {

	assert(cut_vdesc_ptr->seg_ptr != NULL);

	vdesc_cutter_segment(cut_vdesc_ptr->seg_ptr, F_DIRECT);

	/* It's not atomic but it's fine because cutter thread number is only one. */
	cut_vdesc_ptr->next_vdesc_ptr = NULL;

	/* Change the tail pointer */
	VDescNodePtr old_tail = this->garbage_list_tail;
	this->garbage_list_tail = cut_vdesc_ptr;

	remove_file_if_possible(cut_vdesc_ptr->seg_id);

	(void)__sync_lock_test_and_set(&old_tail->next_vdesc_ptr, cut_vdesc_ptr);
}

/* helper function */
bool is_it_dead(Xid v_min, Xid v_max, uint64_t* range, uint64_t len) {
	
	for (uint64_t i = 0; i < len; ++i) {
		if (range[2 * i] < v_min && v_max < range[2 * i + 1])
			return (true);
	}
	return (false);
}

/*
   vdesc_cutter(Xid xid):
xid : Reclaimable maximum version

@return (ret)

FIXME : We should do it in range thing.

 */
void VDesc::cut(uint64_t* range, uint64_t len) {

	// Prev vdesc ptr is pointing to dummy in the first place.
	VDescNodePtr prev_vdesc_ptr = this->head_vdesc_ptr;
	VDescNodePtr cut_vdesc_ptr = this->head_vdesc_ptr -> next_vdesc_ptr;

	while (cut_vdesc_ptr != NULL) {

		if (cut_vdesc_ptr -> v_min != UINT64_MAX) {

			if (cut_vdesc_ptr -> full && is_it_dead(cut_vdesc_ptr->v_min, cut_vdesc_ptr->v_max,
						range, len)) {
				// If we have to cut the segment, free it and change cut_vdesc_ptr only.
				prev_vdesc_ptr->next_vdesc_ptr = cut_vdesc_ptr->next_vdesc_ptr;

				vdesc_cutter_internal(cut_vdesc_ptr);
				this->num_seg_cut_hi += 1;
				cut_vdesc_ptr = prev_vdesc_ptr -> next_vdesc_ptr;

			} else {

				// If we don't have to cut the segment, just skip it.
				prev_vdesc_ptr = cut_vdesc_ptr;

				cut_vdesc_ptr = cut_vdesc_ptr -> next_vdesc_ptr;
			}

		} else {
			prev_vdesc_ptr = cut_vdesc_ptr;
			cut_vdesc_ptr = cut_vdesc_ptr -> next_vdesc_ptr;
		}
	}
}

/*
 vdesc_add_segment(uint32_t seg_no):
    seg_no : segment number (id)
@return (ret)

When new segment is needed in Vdriver, You must call this function too.

*/


void VDesc::add_segment(Sid seg_no) {

	VDescNodePtr ptr = new VDescNode(seg_no, this->num_versions);
	this->tail_vdesc_ptr->next_vdesc_ptr = ptr;
	vdesc_ptr_arr[seg_no] = ptr;
	
	(void)__sync_lock_test_and_set(&this->tail_vdesc_ptr, ptr);
}

Timestamp VDesc::get_timestamp_min() {
	Timestamp min_ts = get_timestamp();
	Timestamp ts;

	for ( auto it : thread_table ) {
		ts = it.timestamp;
		
		if (ts != 0 && ts < min_ts) 
			min_ts = ts;
	}
	
	return min_ts;
}

void VDesc::garbage_collector_func() {
	VDescNodePtr vdesc_ptr = nullptr;
	Timestamp min_ts, ts;
	bool is_deletable;

	while (this->vdesc_status == VDESC_LIVE) {
		
		vdesc_ptr = this->garbage_list_head->next_vdesc_ptr;

		if (vdesc_ptr != nullptr) {
			min_ts = get_timestamp_min();
			is_deletable = true;

			for (int i = 0 ; i < this->num_versions; ++i) {
				ts = vdesc_ptr->seg_ptr[i].timestamp;
				if (ts >= min_ts || ts == 0) {
					is_deletable = false;
					break;
				}
			}

			if (is_deletable) {
				this->num_seg_cut_lo += 1;
				delete (this->garbage_list_head);
				this->garbage_list_head = vdesc_ptr;
				free(vdesc_ptr->seg_ptr);
				vdesc_ptr->seg_ptr = nullptr;
			}
		}

	}
}


void VDesc::remove_file_if_possible(uint32_t seg_id) {
	int ret;

	ret = __sync_fetch_and_add(&seg_flag[seg_id], DEL_MARK);
	
	/* If reference count is not 0, pass it */
	if (ret != 0)
		return;
	/* Else delete file (segment) */
	else {
		this->num_file_cut++;
		// file delete
		char buf[100];
		close(__sync_lock_test_and_set(&seg_fd[seg_id], -2));

		sprintf(buf, "vseg.%.5d", seg_id);
		ret = unlink(buf);
		ut_a(ret == 0);

	}
}

VDesc_sys::VDesc_sys(int num_versions, uint64_t thread_table_size) {
	memset(seg_fd, 0x00, sizeof(seg_fd));
	memset(seg_flag, 0x00, sizeof(seg_flag));

	thread_table.reserve(thread_table_size);
	thread_table.resize(thread_table_size);

	for (int i = 0; i < 3; ++i) {
		VDesc* vdesc_ptr = new VDesc(num_versions);
		vdesc[i] = vdesc_ptr;
	}
}

VDesc_sys::~VDesc_sys() {
	for (int i = 0; i < 3; ++i)
		delete vdesc[i];
}

void VDesc_sys::insert(VersionLoc v_loc, Xid xid_start, Xid xid_end, 
		int seg_loc, SegNodePtr tail_seg_ptr, int clu_no) {

	(void)vdesc[clu_no]->insert(v_loc, xid_start, xid_end,
			seg_loc, tail_seg_ptr);

}


VersionLoc VDesc_sys::lookup(Tid tid, ReadView* read_view, 
		SegNodePtr head_ptr, SegNodePtr tail_ptr, dict_index_t* index) {
	// 1. Set timestamp
	thread_table[tid].timestamp = get_timestamp();

	// 2. Get version locator
	SegNodePtr cur_ptr = tail_ptr;
#ifdef HYU_CHAIN
  read_view->loop_cnt++;
#endif /* HYU_CHAIN */

	while (cur_ptr != head_ptr &&
			(!read_view->changes_visible((trx_id_t)cur_ptr->v_start, index->table->name) || cur_ptr->timestamp != 0)) {
		cur_ptr = cur_ptr->prev_seg_ptr;

#ifdef HYU_CHAIN
		read_view->loop_cnt++;
#endif /* HYU_CHAIN */
	}

	// 3. Unset timestamp
	thread_table[tid].timestamp = 0;
	
	if (cur_ptr != head_ptr)
		return (cur_ptr->v_loc);

	else if (read_view->changes_visible(cur_ptr->v_start, index->table->name))
		return (cur_ptr->v_loc);
	
	else
		return (0);
		
}

int VDesc_sys::check(Tid tid, SegNodePtr head_ptr, SegNodePtr tail_ptr, 
				dict_index_t* index, uint64_t* range, uint64_t len) {
	/** 1. Set timestamp */
	thread_table[tid].timestamp = get_timestamp();

	/** 2. Get version locator */
	SegNodePtr cur_ptr = tail_ptr;
	bool ret = true;

	while	(((ret = is_it_dead(cur_ptr->v_start, cur_ptr->next_seg_ptr->v_start, range, len)) || cur_ptr->timestamp != 0) &&
		cur_ptr != head_ptr) {
		cur_ptr = cur_ptr->prev_seg_ptr;
	}

	if (ret && head_ptr == cur_ptr) {
			ret = is_it_dead(cur_ptr->v_start, cur_ptr->next_seg_ptr->v_start, range, len);
	}
	
	/** 3. Unset timestamp */
	thread_table[tid].timestamp = 0;

	return (ret) ? 1 : 0;
}

void VDesc_sys::cut(uint64_t* range, uint64_t len) {
	
	for (int i = 0; i < 3; ++i)
		(void)vdesc[i]->cut(range, len);

}

void VDesc_sys::add_segment(Sid seg_no, int clu_no) {

	(void)vdesc[clu_no]->add_segment(seg_no);

}

uint64_t VDesc_sys::total_seg_cut_hi() {
	uint64_t ret = 0;
	for (int i = 0; i < 3; ++i)
		ret += vdesc[i]->total_seg_cut_hi();
	return (ret);
}

uint64_t VDesc_sys::total_seg_cut_lo() {
	uint64_t ret = 0;
	for (int i = 0; i < 3; ++i)
		ret += vdesc[i]->total_seg_cut_lo();
	return (ret);
}

uint64_t VDesc_sys::total_file_cut() {
	uint64_t ret = 0;
	for (int i = 0; i < 3; ++i)
		ret += vdesc[i]->total_file_cut();
	return (ret);
}
