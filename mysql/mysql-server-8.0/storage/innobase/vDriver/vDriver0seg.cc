

#include "vDriver0seg.h"

#ifdef HYU 

#include "llb0llb.h"

#include "my_config.h"

#include "btr0btr.h"
#include "buf0buf.h"
#include "fil0fil.h"
#include "fsp0sysspace.h"
#include "ha_prototypes.h"
#include "mem0mem.h"
#include "my_dbug.h"
#include "page0size.h"
#ifndef UNIV_HOTBACKUP
#include "btr0sea.h"
#include "buf0buddy.h"
#include "buf0stats.h"
#include "dict0stats_bg.h"
#include "ibuf0ibuf.h"
#include "log0log.h"
#include "sync0rw.h"

#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <time.h>
#include <map>
#include <new>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "buf0checksum.h"
#include "buf0dump.h"
#include "dict0dict.h"
#include "log0recv.h"
#include "os0thread-create.h"
#include "page0zip.h"
#include "srv0mon.h"
#include "srv0srv.h"
#include "srv0start.h"
#include "sync0sync.h"
#include "ut0new.h"

#endif

#include "ha0ha.h"

/* extern global variables */
extern int seg_fd[];
extern int seg_flag[];

/* vCache system */
vCache_sys_t *vCache_sys = NULL;

/* Location Lookaside buffer system */
LLB *llb_sys = nullptr;

/* Dead zone system */
DEAD *dead = nullptr;

/* Stat system */
STAT *stat_sys = nullptr;

/* Segment id */
vseg_id_t vseg_counter = 1;

/* vCache page manager */
vCache_page_t *vCache;

/* Per-record hash table manager */
hash_table_t *rec_hash;

/* For classification */
ib_uint64_t delta_hot = 0;
ib_uint64_t delta_LLT = 0;
int delta_update = 0;

/* static function used to remove file */
static void remove_file(uint32_t, char*);

/* static function used to open file */
static void open_file(uint32_t, char*);

/* static function used to regist vCache page to page hash  */
static void vDriver_cache_regist(ver_key_t, vCache_page_t*, bool, vpage_id_t);

static int vDriver_classify_ver(vnum_t, vnum_t);

void remove_file(uint32_t seg_no, char* buf) {
	int ret;
	ut_a(seg_no > 0);
	close(__sync_lock_test_and_set(&seg_fd[seg_no], -2));
	sprintf(buf, "vseg.%.5d", seg_no);
	ret = unlink(buf);
	ut_a(ret == 0);
}

void open_file(uint32_t seg_no, char* buf) {
	sprintf(buf, "vseg.%.5d", seg_no);
	seg_fd[seg_no] = open(buf, O_RDWR|O_CREAT, 0644);
	ut_a(seg_fd[seg_no] != -1);

	fallocate(seg_fd[seg_no], 0, 0, VSEG_SIZE);
}

// vDriver_init()
// this function initialize vDriver with allocate vCache,
// vCache_sys, create mutex, hash, and all lists using in vDriver
void vDriver_init() {
	int seg_no;
	char buf[100];
	
	// vCache allocation
	vCache = static_cast<vCache_page_t *>(ut_zalloc_nokey(sizeof(vCache_page_t) * (VCACHE_SIZE / VCACHE_PAGE_SIZE)));

	// vCache_sys allocation
	vCache_sys = static_cast<vCache_sys_t *>(ut_zalloc_nokey(sizeof(vCache_sys_t)));
	
	// mutex, and list creation in vCache_sys
	mutex_create(LATCH_ID_VCACHE_LRU_MUTEX, &vCache_sys->LRU_mutex);
	mutex_create(LATCH_ID_VCACHE_FREE_LIST_MUTEX, &vCache_sys->free_list_mutex);
	UT_LIST_INIT(vCache_sys->LRU, &vCache_page_t::LRU);
	UT_LIST_INIT(vCache_sys->free_list, &vCache_page_t::free_list);
	
	// vCache page hash table create
	vCache_sys->page_hash = ib_create(VCACHE_SIZE / VCACHE_PAGE_SIZE, LATCH_ID_HASH_TABLE_RW_LOCK, HASH_LOCKS, MEM_HEAP_FOR_PAGE_HASH);
	
	// cache initialize
	mutex_enter(&vCache_sys->free_list_mutex);
	for (int i = 0; i < VCACHE_SIZE / VCACHE_PAGE_SIZE; i++) {
		// initialize vCache page and add to free page list
		vCache[i].refcnt = 0;
		vCache[i].is_dirty = false;
		vCache[i].vseg_id = 0;
		vCache[i].vpage_id = 0;
		vCache[i].cluster = -1;
		vCache[i].written_bytes = 0;
		vCache[i].init = 0;
		vCache[i].buffer = (byte*)aligned_alloc(VCACHE_PAGE_SIZE, VCACHE_PAGE_SIZE);
		UT_LIST_ADD_LAST(vCache_sys->free_list, &vCache[i]);
	}
	mutex_exit(&vCache_sys->free_list_mutex);
	
	/* LLB system initialization */
	llb_sys = new LLB((int)NUM_VER_IN_SEG, (int)NUM_REC, (uint64_t)NUM_WORKER_THREAD);
	
	// segment initialize
	// create one vSegment per cluster
	// and allot pages for vSegment
	for (int i = 0; i < NUM_VCLUSTER; i++) {
		// get vSegment id
		seg_no = __sync_fetch_and_add(&vseg_counter, 1);
		
		// create vSegment file
		open_file(seg_no, buf);
		
		// cache allotment for vSegment
		vDriver_cache_allot(seg_no, UINT32_MAX, VSEG_SIZE / VCACHE_PAGE_SIZE);
		// You should also create LLB things
		
		llb_sys->add_segment((uint32_t)seg_no, i);
	}
	
	// per-record hash table create
	rec_hash = ib_create(NUM_REC, LATCH_ID_HASH_TABLE_RW_LOCK, HASH_LOCKS, MEM_HEAP_FOR_PAGE_HASH);

}

// vDriver_shutdown
// clear vDriver when innoDB server termination
void vDriver_shutdown() {
	char buf[100];

	// close all vSegment files and delete files
	for (int i = 0; i < NUM_VSEG; i++) {
		if (seg_fd[i] > 0) {
			remove_file(i, buf);
		}
	}


	// clear free page list with page buffer free
	int list_len = UT_LIST_GET_LEN(vCache_sys->free_list);
	vCache_page_t *page;
	for (int i = 0; i < list_len; i++) {
		page = UT_LIST_GET_FIRST(vCache_sys->free_list);
		free(page->buffer);
		UT_LIST_REMOVE(vCache_sys->free_list, page);
	}


	// clear LRU list with page buffer free
	list_len = UT_LIST_GET_LEN(vCache_sys->LRU);
	for (int i = 0; i < list_len; i++) {
		page = UT_LIST_GET_FIRST(vCache_sys->LRU);
		free(page->buffer);
		UT_LIST_REMOVE(vCache_sys->LRU, page);
	}

	
	// free vCache
	ut_free(vCache);


	// make vCache page hash table clear and free
	ha_clear(vCache_sys->page_hash);
	hash_table_free(vCache_sys->page_hash);
	
	// destroy mutex
	mutex_free(&vCache_sys->LRU_mutex);
	mutex_free(&vCache_sys->free_list_mutex);
	
	// free vCache_sys
	ut_free(vCache_sys);
	ha_clear(rec_hash);
	hash_table_free(rec_hash);

	delete (llb_sys);

}

// vDriver_add_segment
// make additional segment with cache allotment
// see vDriver_init() for get some help to understand this function
// @param[in] cluster			: which cluster the segment in
// @param[out]							: if success, return segment_id. -1 in error
vseg_id_t vDriver_add_segment(uint8_t cluster) {
	int seg_no;
	char buf[100];
	ibool err;

	INCR_STAT(stat_sys, num_seg[cluster]);

	seg_no = __sync_fetch_and_add(&vseg_counter, 1);
	
	open_file(seg_no, buf);
	ut_a(seg_no > 0);	
	err = vDriver_cache_allot(seg_no, UINT32_MAX, NUM_VSEG_PAGE);
	ut_a(err);

	llb_sys->add_segment((uint32_t)seg_no, cluster);

	return (seg_no);
}

// 0->free page, 1->LRU
void vDriver_cache_regist(ver_key_t key, vCache_page_t *page, bool flag, vpage_id_t read_flag) {
	rw_lock_t *hash_lock;
	rec_t *data;
	vseg_id_t vseg_id;
	vpage_id_t vpage_id;

	vseg_id = SEG_ID(key);
	ut_a(vseg_id != 0);
	vpage_id = OFFSET(key);

	// register to vCache page hash table
	hash_lock = hash_get_lock(vCache_sys->page_hash, key);
	rw_lock_x_lock(hash_lock);
	data = (rec_t *)ha_search_and_get_data(vCache_sys->page_hash, key);
	if (data != NULL) {
		rw_lock_x_unlock(hash_lock);
		
		if (flag) {
			mutex_enter(&vCache_sys->LRU_mutex);
			UT_LIST_ADD_FIRST(vCache_sys->LRU, page);
			mutex_exit(&vCache_sys->LRU_mutex);
		} else {
			mutex_enter(&vCache_sys->free_list_mutex);
			UT_LIST_ADD_FIRST(vCache_sys->free_list, page);
			mutex_exit(&vCache_sys->free_list_mutex);
		}

	} else {
		
		if (read_flag == UINT32_MAX)
			page = vDriver_init_page(page, 1, false, vseg_id, vpage_id, 0, 0);
		else
			page = vDriver_init_page(page, 0, false, vseg_id, vpage_id, 0, 1);

		ha_insert_for_fold(vCache_sys->page_hash, key, NULL, (rec_t*)page);			
		rw_lock_x_unlock(hash_lock);

		mutex_enter(&vCache_sys->LRU_mutex);
		UT_LIST_ADD_LAST(vCache_sys->LRU, page);
		mutex_exit(&vCache_sys->LRU_mutex);
	}
}


// vDriver_cache_allot
// allot vCache for read/write
// @param[in] vseg_id		:	vSegment id
// @param[in] vpage_id		: if -1, adding new segment 
//							  if >=0, allot pages for read
// @param[in] page_num		: number of pages to allot
// @param[out]				: return true when allotment success
//							  return false in error case
bool vDriver_cache_allot(vseg_id_t vseg_id, vpage_id_t vpage_id, ib_uint32_t page_num) {
	vCache_page_t *page;
	ver_key_t key;
	
	// cache allotment
	// get page from free page list first
	// if free page exists, init page and using them
	// if free page doesn't exist, evict page using LRU
	// then add LRU list
	for (ib_uint32_t i = 0; i < page_num; i++) {
		
		if (vpage_id == UINT32_MAX) { 
			key = vseg_id;
			key = key << 32;
			key |= i;
		} else {
			key = vseg_id;
			key = key << 32;
			key |= vpage_id;
		}

		mutex_enter(&vCache_sys->free_list_mutex);
		page = UT_LIST_GET_FIRST(vCache_sys->free_list);

		// free pages exist
		if (page != NULL) {
			UT_LIST_REMOVE(vCache_sys->free_list, page);
			mutex_exit(&vCache_sys->free_list_mutex);
			
			vDriver_cache_regist(key, page, false, vpage_id);
			
		} else { // if all vCache page is used
			mutex_exit(&vCache_sys->free_list_mutex);
			page = vDriver_evict_page();
			ut_a(page != NULL);
		
			vDriver_cache_regist(key, page, true, vpage_id);
		}
	}
	return true;
}

// vDriver_init_page
// initialize vCache page
// @param[in, out] page			: page for initialize
// @param[in] refcnt				: page ref count
// @param[in] pinned				: normal case, pinned always true
// @param[in] is_dirty			: if this page is dirty, set true
// @param[in] vseg_id				: vSegment id for this page
// @param[in] vpage_id			: page id of vSegment
// @param[in] written_bytes	: written bytes of buffer. 0 in normal case
vCache_page_t* vDriver_init_page(vCache_page_t *page, ib_uint64_t refcnt, bool is_dirty, vseg_id_t vseg_id, vpage_id_t vpage_id, ib_uint32_t written_bytes, int init) {
	ut_a(page != NULL);
	page->refcnt = refcnt;
	page->is_dirty = is_dirty;
	page->vseg_id = vseg_id;
	page->vpage_id = vpage_id;
	page->written_bytes = written_bytes;
	memset(page->buffer, 0, VCACHE_PAGE_SIZE);

	return page;
}


// vDriver_write_dirty
// write dirty page to storage
// this function only called by vDriver_evict_page()
// @param[in] page	: page to write
// @param[out]			: return true if write success
bool vDriver_write_dirty(vCache_page_t *page) {
	int err;
	int ret;
	char buf[100];


	INCR_STAT(stat_sys, total_page_write);
	
	void* v_ptr = nullptr;
	memset(buf, 0x00, sizeof(buf));
	vnum_t v_min = UINT64_MAX;
	vnum_t v_max = 0;

	/* We should find the v_min and v_max value in here */
	for (int i = 0; i < VCACHE_PAGE_SIZE / VER_LEN; ++i) {
		/* Set v_min value */
		v_ptr = page->buffer + VER_LEN * i + V_MIN_OFFSET;
		if (v_min > *(vnum_t*)v_ptr)
			v_min = *(vnum_t*)v_ptr;

		/* Set v_max value */
		v_ptr = page->buffer + VER_LEN * i + V_MAX_OFFSET;
		if (v_max < *(vnum_t*)v_ptr)
			v_max = *(vnum_t*)v_ptr;
	}

	/* 2nd pruning (page unit) */
	if (dead->can_pruning(v_min, v_max)){
		__sync_fetch_and_add(&stat_sys->second_pr_cluster[page->cluster], VCACHE_PAGE_SIZE / VER_LEN);
		INCR_STAT(stat_sys, second_pruning);
		return (true);
	}

	/* 2nd pruning (segment unit) */
	/**
	  32-bit flag per segment
	   
	  1 left-most bit
	    DEL_MARK ( 0 for live, 1 for delete )
	  
	  31 remain bits
	    Reference count for reader
	 
	  */

	/* 1. Add and fetch its flag */
	ret = __sync_add_and_fetch(&seg_flag[page->vseg_id], 1);
	
	/* 2. If the file is alive */
	if ((ret & DEL_MARK) == 0) {

		/* 2-1 : Write its buffer to file */
		//bytes = write(seg_fd[page->vseg_id], page->buffer, VCACHE_PAGE_SIZE);
		err = my_pwrite(seg_fd[page->vseg_id], page->buffer, (size_t)VCACHE_PAGE_SIZE,
				(my_off_t)(page->vpage_id * VCACHE_PAGE_SIZE), MY_NABP);
		ut_a(err == 0);
		
		/* 2-2 : Sub and fetch its flag */
		ret = __sync_sub_and_fetch(&seg_flag[page->vseg_id], 1);
		
		/* 2-3 : Delete file if needed */
		if ((ret & DEL_MARK) == 0) {
			// do nothing
		} else if ((ret & REF_MARK) != 0) {
			// file will be deleted by other worker
			return (true);
		} else {
			// I should delete file !! It's my job.
			remove_file(page->vseg_id, buf);
			INCR_STAT(stat_sys, total_file_cut);
		}
	/* 3. If the file is deleted or deleting */
	} else {
		if (ret == SKIP_MARK) {
			// do nothing
		} else {
			/* 3-1 : Sub and fetch its flag */
			ret = __sync_sub_and_fetch(&seg_flag[page->vseg_id], 1);

			/* 3-2 : Delete file if needed */
			if ((ret & REF_MARK) == 0){
				// I should delete file !! It's my job
				remove_file(page->vseg_id, buf);
				INCR_STAT(stat_sys, total_file_cut);
			}
		}
		return (true);
	}

	return (true);
}

/**
vDriver_read_page(vCache_page_t *page):
	Read page from disk.
*/
bool vDriver_read_page(vCache_page_t *page) {
	int err;
	
	err = my_pread(seg_fd[page->vseg_id], page->buffer, (size_t)VCACHE_PAGE_SIZE, 
			(my_off_t)(page->vpage_id * VCACHE_PAGE_SIZE), MY_NABP);
	
	ut_a(err == 0);	
	
	return true;
}

// vDriver_free_page
// add vCache page to free page list
// @param[in] page	: page to add free list
// @param[out]			: retrun true if add success
bool vDriver_free_page(vCache_page_t *page) {
	mutex_enter(&vCache_sys->free_list_mutex);
	UT_LIST_ADD_LAST(vCache_sys->free_list, page);
	mutex_exit(&vCache_sys->free_list_mutex);

	return true;
}

// vDriver_evict_page
// evict one page using LRU algorithm
// @param[out]	: a page that occur eviction
vCache_page_t* vDriver_evict_page() {
	vCache_page_t *page = NULL;
	ver_key_t key;
	rw_lock_t *hash_lock;
	
	// get page to evict in LRU list
	mutex_enter(&vCache_sys->LRU_mutex);
	page = UT_LIST_GET_FIRST(vCache_sys->LRU);
	while(1) {
		key = page->vseg_id;
		ut_a(key != 0);
		key = key << 32;
		key |= page->vpage_id;

		hash_lock = hash_get_lock(vCache_sys->page_hash, key);

		if (__sync_fetch_and_add(&page->refcnt, 0) == 0 && __sync_fetch_and_add(&page->init, 0) != 1) {
			rw_lock_x_lock(hash_lock);
			if (__sync_fetch_and_add(&page->refcnt, 0) == 0 && __sync_fetch_and_add(&page->init, 0) != 1) {
				break;	
			}
			rw_lock_x_unlock(hash_lock);
		} else {
			page = UT_LIST_GET_NEXT(LRU, page);
		}

		if (page == NULL) {
			mutex_exit(&vCache_sys->LRU_mutex);
		}
		ut_a(page != NULL);
	}
	// if get page from LRU list, remove page node in LRU
	UT_LIST_REMOVE(vCache_sys->LRU, page);
	mutex_exit(&vCache_sys->LRU_mutex);
	
	// write page if page is dirty
	if (page->is_dirty)
		vDriver_write_dirty(page);
	
	// get hash lock and delete hash node
	ha_search_and_delete_if_found(vCache_sys->page_hash, key, (rec_t*)page);
	rw_lock_x_unlock(hash_lock);

	return page;
}

vCache_page_t* vDriver_get_page_slow(ver_key_t key, vseg_id_t seg_no, vpage_id_t vpage_id) {
	vCache_page_t *page;
	bool ret;
	
	ret = vDriver_cache_allot(seg_no, vpage_id, 1);
	ut_a(ret);

	page = vDriver_get_page(key);
	(void)__sync_fetch_and_sub(&page->init, 1);
	//__sync_bool_compare_and_swap(&page->init, 1, 0);
	ut_a(page != NULL);

	ret = vDriver_read_page(page);
	ut_a(ret);

	return (page);
}


// vDriver_get_page
// get page from vCache using vCache page hash table
// @param[in] key		: key for hash table
// @param[out]			: return page if success, NULL if fail
vCache_page_t* vDriver_get_page(ver_key_t key) {
	rw_lock_t *hash_lock;
	vCache_page_t *page;
	rec_t *data;
	
	// get hash lock and get vCache page
	hash_lock = hash_get_lock(vCache_sys->page_hash, key);
	rw_lock_s_lock(hash_lock);
	data = (rec_t*)ha_search_and_get_data(vCache_sys->page_hash, key);
	if (data == NULL) {
		rw_lock_s_unlock(hash_lock);
		return (NULL);
	}
	page = (vCache_page_t*) data;
	
	(void)__sync_fetch_and_add(&page->refcnt, 1);
	
	rw_lock_s_unlock(hash_lock);

	return page;
}

// vDriver_insert_ver
// @param[in] vnum		: version number of version
// @param[in] cluster	: cluster number of version
// @param[in] version	: version that needs to insert
// @param[out] 				: version length if insertion success
int vDriver_insert_ver(vnum_t v_start, vnum_t v_end, vVersion *version,
												ib_uint32_t space_id) {
	
	vpage_id_t vpage_id;
	ver_key_t key = 0;
	vCache_page_t *page;
	vseg_id_t vseg_id, err;
	ib_uint32_t vseg_offset, written_bytes, page_offset;
	rw_lock_t *hash_lock;
	rec_t *data;
	SegNode *segnode;
	ver_key_t locater;
	int nth_in_seg;
	uint8_t cluster;
	VDescNodePtr tail_vdesc_ptr;
	
	cluster = (uint8_t)vDriver_classify_ver(v_start, v_end);

	// 1st pruning in here.
	
	INCR_STAT(stat_sys, total_insert);
	INCR_STAT(stat_sys, total_ver_cluster[cluster]);
	if (dead->can_pruning(v_start, v_end)){
		INCR_STAT(stat_sys, first_pr_cluster[cluster]);
		INCR_STAT(stat_sys, first_pruning);
		return (1);
	}

retry:	
	tail_vdesc_ptr = llb_sys->get_tail_ptr(cluster);
	vseg_id = tail_vdesc_ptr->seg_id;
	ut_a(vseg_id != 0);
	ut_a(tail_vdesc_ptr != NULL);
	
	vseg_offset = __sync_fetch_and_add(&tail_vdesc_ptr->written_bytes, VER_LEN);
	
	if (vseg_offset + VER_LEN == VSEG_SIZE) {
		err = vDriver_add_segment(cluster);
		ut_a(err != 0);
	} else if (vseg_offset >= VSEG_SIZE) {
		tail_vdesc_ptr->written_bytes = VSEG_SIZE;
	
		while (1) {
			sleep(0.001);
			tail_vdesc_ptr = llb_sys->get_tail_ptr(cluster);
			if (vseg_id != tail_vdesc_ptr->seg_id)
				break;
		}
		goto retry;	
	
	} else {
		// do nothing
	}

	// get page id of the version
	// and generate key to get vCache page
	vpage_id = vseg_offset / VCACHE_PAGE_SIZE;
	ut_a(vpage_id <= NUM_VSEG_PAGE);
	key = tail_vdesc_ptr->seg_id;
	ut_a(key != 0);
	key = key << 32;
	key |= vpage_id;

	// get page for insert version from vCache page hash table
	page = vDriver_get_page(key);
	ut_a(page != NULL);

	// search physical location of the version
	// then store it using memcpy
	page_offset = vseg_offset % VCACHE_PAGE_SIZE;

	memcpy(version->version + V_MIN_OFFSET, &v_start, sizeof(vnum_t));
	memcpy(version->version + V_MAX_OFFSET, &v_end, sizeof(vnum_t));
	memcpy(page->buffer + page_offset, version->version, VER_LEN); // version->length
	
	written_bytes = __sync_add_and_fetch(&page->written_bytes, VER_LEN);

	if (page_offset == 0) {
		page->cluster = cluster;
		page->is_dirty = true;
	}

	// get hash lock and get llb structure
	ib_uint64_t rec_pri_key = 0;
	ib_uint64_t tmp_key;
	int num_shift_bit;
	for (ulint i = 0; i < version->pk_len; i++) {
		tmp_key = version->primary_key[i];
		num_shift_bit = 8 * (version->pk_len - (i + 1));
		tmp_key = tmp_key << num_shift_bit;
		rec_pri_key |= tmp_key;
	}
	rec_pri_key = rec_pri_key << 32;
	rec_pri_key |= space_id;
	

	hash_lock = hash_get_lock(rec_hash, rec_pri_key);
	rw_lock_s_lock(hash_lock);
	data = (rec_t*)ha_search_and_get_data(rec_hash, rec_pri_key);

	if (data == NULL) {
		rw_lock_s_unlock(hash_lock);
		rw_lock_x_lock(hash_lock);
		segnode = (SegNode*)ut_zalloc_nokey(sizeof(SegNode));
		segnode->prev_seg_ptr = segnode;
		segnode->next_seg_ptr = segnode;
		segnode->v_start = UINT64_MAX;

		ha_insert_for_fold(rec_hash, rec_pri_key, NULL, (rec_t*)segnode);
		
		rw_lock_x_unlock(hash_lock);
	} else {
		segnode = (SegNode*)data;
		
		rw_lock_s_unlock(hash_lock);
	}
	ut_a(segnode != NULL);
	// refresh LRU list for this page
	mutex_enter(&vCache_sys->LRU_mutex);
	UT_LIST_REMOVE(vCache_sys->LRU, page);
	UT_LIST_ADD_LAST(vCache_sys->LRU, page);
	mutex_exit(&vCache_sys->LRU_mutex);
	
	// if full page is written, unpin this page
	// this page can evict
	if (written_bytes == VCACHE_PAGE_SIZE){
		(void)__sync_sub_and_fetch(&page->refcnt, 2);
	} else {
		(void)__sync_sub_and_fetch(&page->refcnt, 1);
	}
	

	locater = tail_vdesc_ptr->seg_id;
	locater = locater << 32;
	locater |= vseg_offset;
	nth_in_seg = vseg_offset / VER_LEN;
	
	llb_sys->insert(segnode, locater, (uint64_t)v_start, (uint64_t)v_end,nth_in_seg, cluster);
	
	return (version->length);
}

vDriver_delta_t vDriver_update_delta(vDriver_delta_t temp) {
	vDriver_delta_t delta;
	double tmp;

	// update global delta
	// TODO: race condition
	if (delta_hot == 0) {
		delta_hot = temp.vlife;
	} else {
		tmp = (double)delta_hot * W_AVG + (double)temp.vlife * W_NEW;
		delta_hot = (uint64_t)tmp;
	}
	
	
	delta.vlife = delta_hot;
	delta.tlife = delta_LLT;

	return delta;
}

// return cluster number
int vDriver_classify_ver(vnum_t v_start, vnum_t v_end) {
	vDriver_delta_t temp;
	vDriver_delta_t delta;
	deadzone* zone;
	uint64_t range[2 * NUM_DEADZONE];
	uint64_t len;
	uint64_t max_trx_id;
	uint64_t guideline;
	uint64_t finalline = UINT64_MAX;
	uint64_t LLT = UINT64_MAX;
	uint64_t trx_list[NUM_WORKER_THREAD];
	int passer;

	// calculate version lifetime
	if (v_end > v_start)
		temp.vlife = v_end - v_start;
	else
		temp.vlife = v_start - v_end;

	passer = __sync_lock_test_and_set(&delta_update, 1);

	if (!passer) {
		delta = vDriver_update_delta(temp);
		delta_update = 0;
	}
	else {
		delta.vlife = delta_hot;
		delta.tlife = delta_LLT;
	}

	// classification : check if there are some LLTs
	// TODO: check if length of DEAD_ZONE > delta_LLT * MUL_LLT (except DEAD_ZONE 0)
	//			 if exist, there will be some Long Transactions
	//			 					 see DEAD_ZONE 1 then check zone_start > v_start
	//																YES: goto VC_LLT // NO: check HOT / COLD
	//			 if not exist, check HOT / COLD
	dead->rw_s_lock();
	(void)__sync_fetch_and_add(&(dead->get_cur_zone()->ref_cnt), 1);
	zone = dead->get_cur_zone();
	dead->rw_s_unlock();

	ut_a(zone->len <= NUM_DEADZONE);

	int trx_list_len = zone->oldest_active_trx_list_len;
    if (trx_list_len >= 0) { 
        LLT = zone->oldest_low_limit_id;
        memcpy(trx_list, zone->oldest_active_trx_list, sizeof(uint64_t) * trx_list_len);
    }    

	/* The only thing to do is memcpy()! - copy current deadzone */
	memcpy(range, zone->range, sizeof(uint64_t) * zone->len * 2);
	len = zone->len;

	(void)__sync_fetch_and_sub(&(zone->ref_cnt), 1);

	max_trx_id = trx_sys_get_max_trx_id();

	guideline = max_trx_id - delta.tlife * MUL_LLT;

	/* Cold First */
	if (temp.vlife > delta.vlife * MUL_HOT)
		return (VC_COLD);

	for (ulint i = len - 1; i > 0; --i) {
		if (range[i * 2] <= guideline && range[i * 2] < finalline) {
			finalline = range[i * 2];
			break;
		}
	}

	if (finalline == UINT64_MAX){
			return (VC_HOT);
	}

	if (v_start <= finalline) {
		if (v_end < LLT) {
			if (v_end < range[1])
				return (VC_HOT);
			for (int i = 0; i < trx_list_len; ++i) {
				if (v_end == trx_list[i])
					return (VC_LLT);
			}
			return (VC_HOT);	
		}
		return (VC_LLT);
	}
	return (VC_HOT);
}

/** Read a previous version of a clustered index record for a consistent read
@param[in]	read_view		read view
@param[in]	primary_key		primary key
@param[in]	pk_len			primary key length
@param[in]	old_vers		old version, or NULL if the record does not
								exist in the view: i.e., it was error in vdriver model.
@param[in]	tid				thread id
@param[in]	index			index
@return 0 or err. */

int vDriver_read_ver(ReadView* read_view, const byte* primary_key, 
		ulint pk_len, rec_t **old_vers, my_thread_id tid, dict_index_t* index) {
	DECL_RET;
	ib_uint64_t rec_pri_key;
	ib_uint64_t tmp_key;
	rw_lock_t *hash_lock;
	int num_shift_bit;
	rec_t* data;
	SegNode* segnode;
	vpage_id_t vpage_id;
	ver_key_t key;
	vCache_page_t *page;
	vseg_id_t seg_no;
	ib_uint32_t space_id = index->space;
	
	ut_a(tid < NUM_WORKER_THREAD);
	
	rec_pri_key = 0LL;
	for (ulint i = 0; i < pk_len; i++) {
		tmp_key = primary_key[i];
		num_shift_bit = 8 * (pk_len - (i + 1));
		tmp_key = tmp_key << num_shift_bit;
		rec_pri_key |= tmp_key;
	}
	
	rec_pri_key = rec_pri_key << 32;
	rec_pri_key |= space_id;
	
	// 1. Get segment node pointer from per-record hash table using by primary key.
	hash_lock = hash_get_lock(rec_hash, rec_pri_key);
	rw_lock_s_lock(hash_lock);
	data = (rec_t*)ha_search_and_get_data(rec_hash, rec_pri_key);
	rw_lock_s_unlock(hash_lock);
	
	segnode = (SegNode*)data;
	ut_a(segnode != nullptr);

	// 2. Get version locator from Location Lookaside Buffer (LLB)
	VersionLoc v_loc = llb_sys->lookup(segnode, (uint64_t)tid, read_view, index);
	ut_a(v_loc != 0);

	/* Get vpage_id, segment id, key from version locator */
	vpage_id = (vpage_id_t)(OFFSET(v_loc) / VCACHE_PAGE_SIZE);
	seg_no = (vseg_id_t)(SEG_ID(v_loc));	

	ut_a(seg_no > 0);
	key = SEGMENT(v_loc) | vpage_id;
	ut_a(key == (((v_loc >> 32) << 32) | vpage_id));


	// 3. Get old-version record from Version Cluster (VC)
	// A> getpage
	// B> if not, first do cache allot and
	// C>          get page again.
	
	page = vDriver_get_page(key);
	
	// This path is not gonna be called.
	// TODO: You have to check out of memory cases.
	if (page == NULL) {
		page = vDriver_get_page_slow(key, seg_no, vpage_id);
	}
	
	ut_a(page != NULL);
	
	ib_uint32_t page_offset = OFFSET(v_loc) % VCACHE_PAGE_SIZE;

	// 4. Read a record from page and store it in old_vers
	//(void)ut_memcpy(*old_vers, page->buffer + page_offset, REC_LEN);
	*old_vers = vDriver_get_record(page, page_offset);
	(void)__sync_sub_and_fetch(&page->refcnt, 1);
	return (ret);
}

/**
Read a record from given page and do memcpy.
@param[in]	page			page
@param[in]	page_offset		page_offset
@return (record) */
rec_t* vDriver_get_record(vCache_page_t* page, ib_uint32_t page_offset) {
	
	byte* buf = (byte*)ut_malloc_nokey(sizeof(byte) * REC_LEN);
	ut_memcpy(buf, page->buffer + page_offset, REC_LEN);

	return ((byte*)buf);
}

/** Read a previous version of a clustered index record for a consistent read
@param[in]	primary_key		primary key
@param[in]	pk_len				primary key length
@param[in]	trx_id				transaction id from index
@param[in]	tid						thread id
@param[in]	index					index
@return 0 or err. */

int vDriver_check_removability(const byte* primary_key, ulint pk_len, 
				trx_id_t trx_id_n, trx_id_t trx_id_o, my_thread_id tid, dict_index_t* index) {
	DECL_RET;
	ib_uint64_t rec_pri_key;
	ib_uint64_t tmp_key;
	rw_lock_t *hash_lock;
	int num_shift_bit;
	rec_t* data;
	SegNode* segnode;
	//vpage_id_t vpage_id;
	//ver_key_t key;
	//vCache_page_t *page;
	//vseg_id_t seg_no;
	ib_uint32_t space_id = index->space;

	ut_a(tid < NUM_WORKER_THREAD);
	
	deadzone* zone = nullptr;
	uint64_t range[2 * NUM_DEADZONE];
	uint64_t len = 0;

	dead->rw_s_lock();
	if (dead->get_cur_zone()) {
			(void)__sync_fetch_and_add(&(dead->get_cur_zone()->ref_cnt), 1);
			zone = dead->get_cur_zone();
	}
	dead->rw_s_unlock();
	
	if (zone) {
		ut_a(zone->len <= NUM_DEADZONE);
		memcpy(range, zone->range, sizeof(uint64_t) * zone->len * 2);
		len = zone->len;
		(void)__sync_fetch_and_sub(&(zone->ref_cnt), 1);
	}
	
	if (!is_it_dead(trx_id_o, trx_id_n, range, len)) {
			return (0);
	}

	rec_pri_key = 0LL;
	for (ulint i = 0; i < pk_len; i++) {
		tmp_key = primary_key[i];
		num_shift_bit = 8 * (pk_len - (i + 1));
		tmp_key = tmp_key << num_shift_bit;
		rec_pri_key |= tmp_key;
	}
	
	rec_pri_key = rec_pri_key << 32;
	rec_pri_key |= space_id;
	
	// 1. Get segment node pointer from per-record hash table using by primary key.
	hash_lock = hash_get_lock(rec_hash, rec_pri_key);
	rw_lock_s_lock(hash_lock);
	data = (rec_t*)ha_search_and_get_data(rec_hash, rec_pri_key);
	rw_lock_s_unlock(hash_lock);
	
	segnode = (SegNode*)data;
	
	if (segnode == nullptr) {
			return (1);
	}
	ut_a(segnode != nullptr);
	
	vnum_t temp_v = segnode->v_start;
	segnode->v_start = trx_id_o;
	/* Check removability */
	ret = llb_sys->check(segnode, (uint64_t)tid, index, range, len);
	segnode->v_start = temp_v;
	
	return (ret);
}


STAT::STAT() : first_pruning(0), second_pruning(0), total_insert(0), total_page_write(0), total_file_cut(0) {
    memset(num_seg, 0x00, sizeof(uint64_t) * 3);
    memset(first_pr_cluster, 0x00, sizeof(uint64_t) * 3);
    memset(second_pr_cluster, 0x00, sizeof(uint64_t) * 3);
    memset(total_ver_cluster, 0x00, sizeof(uint64_t) * 3);
}

STAT::~STAT(){
	//do nothing
}

DEAD::DEAD() {
	this->view_list = new ReadView[NUM_WORKER_THREAD]();
	this->length = 0;
	this->cur_zone = nullptr;

	rw_lock_create(dead_sys_latch_key,&this->latch,SYNC_DEAD_RWLOCK);
	
	UT_LIST_INIT(m_free, &deadzone::m_dead_list);
	UT_LIST_INIT(m_dead, &deadzone::m_dead_list);

	for (int i = 0; i < LEN_DEAD_LIST; ++i) {
		deadzone *dead = UT_NEW_NOKEY(deadzone());

		UT_LIST_ADD_FIRST(m_free, dead);
	}
}

DEAD::~DEAD() {
	rw_lock_free(&this->latch);
	
	delete[] this->view_list;


	for (deadzone* dead = UT_LIST_GET_FIRST(m_free); dead != NULL;
			dead = UT_LIST_GET_FIRST(m_free)){
		UT_LIST_REMOVE(m_free, dead);

		UT_DELETE(dead);
	}

	for (deadzone* dead = UT_LIST_GET_FIRST(m_dead); dead != NULL;
			dead = UT_LIST_GET_FIRST(m_dead)) {
		UT_LIST_REMOVE(m_dead, dead);
		
		UT_DELETE(dead);
	}

	ut_a(UT_LIST_GET_LEN(m_dead) == 0);
	ut_a(UT_LIST_GET_LEN(m_free) == 0);
}

/**
Get free deadzone node from free-list */
deadzone *DEAD::get_dead() {
	ut_a(UT_LIST_GET_LEN(m_free) > 0);

	deadzone *dead;
	dead = UT_LIST_GET_FIRST(m_free);
	ut_a(dead != nullptr);
	UT_LIST_REMOVE(m_free, dead);

	return (dead);
}

ReadView* DEAD::get_view_list() {
	return (view_list);
}

ulint* DEAD::get_length() {
	return &(length);
}

/**
update dead zone */
void DEAD::update_dead_zone() {

    ut_a(this->length != 0);

    /* 1. Get free deadzone structure */
    deadzone* zone = get_dead();

    /* 2. Update "zone 1" */
    zone->range[0] = 0;
    zone->range[1] = view_list[0].up_limit_id();
    zone->len = 1;

    trx_id_t low_limit_id;
    trx_id_t dead_up_limit_id;
	trx_id_t max_trx_id = trx_sys_get_max_trx_id();
    /* 3. Update other deadzone */
    for (ulint i = 1; i < this->length; ++i) {
		
		low_limit_id = view_list[i-1].low_limit_id();
        dead_up_limit_id = view_list[i].dead_up_limit_id(low_limit_id);

        zone->range[2 * zone->len] = low_limit_id;
        zone->range[2 * zone->len + 1] = dead_up_limit_id;
        zone->len++;
    }
    
	ut_a(zone->len <= NUM_DEADZONE);
	/* 3-1. Update "zone 1"s active transaction list */
	if (zone->len == 1 || max_trx_id - (delta_LLT * MUL_LLT) < view_list[0].low_limit_id()) {
		zone->oldest_active_trx_list_len = -1;
	} else {
		view_list[0].copy_active_trx_list(zone->oldest_active_trx_list, zone->oldest_active_trx_list_len);
		zone->oldest_low_limit_id = view_list[0].low_limit_id();
	}


    /* 4. Add it to deadzone list as last one */
    UT_LIST_ADD_LAST(m_dead, zone);

    rw_x_lock();
    /* 5. Change current deadzone pointer */
    this->cur_zone = zone;
    rw_x_unlock();

    ut_a(this->cur_zone == UT_LIST_GET_LAST(m_dead));

    /* 6. Do garbage collection */
    /* FIXME :: Now it works garbage collector itself */
    /*          But I think it can be done by itself  */

    for (deadzone *dz = UT_LIST_GET_FIRST(m_dead); dz != NULL && dz != this->cur_zone;
            dz = UT_LIST_GET_NEXT(m_dead_list, dz)) {
        if (dz->ref_cnt == 0) {

            UT_LIST_REMOVE(m_dead, dz);

            UT_LIST_ADD_LAST(m_free, dz);
        }
    }
    return;
}

bool DEAD::can_pruning(vnum_t v_start, vnum_t v_end) {
	bool ret = false;
	bool flag = true;
	/* Empty deadzone */
	if (this->cur_zone == nullptr)
			return (ret);

	/* TODO:: Use rw_s_lock() to get cur_zone and add its ref_cnt */
	rw_s_lock();
	(void)__sync_fetch_and_add(&(this->cur_zone->ref_cnt), 1);
	deadzone* zone = this->cur_zone;
	rw_s_unlock();

	/* Compare its v_start & v_end to deadzone */
	for (uint64_t i = 0; i < zone->len; ++i) {
		if (i == 0 && zone->oldest_active_trx_list_len != -1) {
			if (v_end < zone->range[1]) {
					ret = true;
					break;
			} else if (v_end >= zone->oldest_low_limit_id) {
					continue;
			}

			for (int j = 0; j < zone->oldest_active_trx_list_len; ++j) {
					if (zone->oldest_active_trx_list[j] == v_end) {
							flag = false;
							break;
					}
			}
			if (flag) {
					ret = true;
					break;
			}
		} else {
			if (zone->range[2 * i] < v_start && 
					v_end < zone->range[2 * i + 1]){ 

				ret = true;
				break;
			}    
		}
	}    
	(void)__sync_fetch_and_sub(&(zone->ref_cnt), 1);  
	return (ret);
}


/**
This function is the main one for "Cut thread". */
void vdriver_cut_thread() {
	deadzone* zone;
	uint64_t range[2 * NUM_DEADZONE];
	uint64_t len;

	while (srv_shutdown_state.load() == SRV_SHUTDOWN_NONE) {
		
		/* Every N sec (not exactly), it executes. */
		os_event_wait_time(srv_vdriver_cut_event, MICROSECS_IN_A_SECOND / 1000);

		/* If the system is shutdown, break the loop. */
		if (srv_shutdown_state.load() != SRV_SHUTDOWN_NONE) {
			break;
		}

		/* Memory barrier */
		os_rmb;
		
		/* Empty deadzone */
		/*if (dead->get_cur_zone() == nullptr)
			continue;
		*/
		/* Get current deadzone and add its ref_cnt */
		dead->rw_s_lock();
		if (dead->get_cur_zone() == nullptr) {
				dead->rw_s_unlock();
				continue;
		}
		(void)__sync_fetch_and_add(&(dead->get_cur_zone()->ref_cnt), 1);
		zone = dead->get_cur_zone();
		dead->rw_s_unlock();

		ut_a(zone -> len <= NUM_DEADZONE);

		/* The only thing to do is memcpy()! - copy current deadzone */
		memcpy(range, zone->range, sizeof(uint64_t) * zone->len * 2);
		len = zone->len;

		(void)__sync_fetch_and_sub(&(zone->ref_cnt), 1);
	
		/* Execute cutter */
		llb_sys->cut(range, len);	
	}
}


void vdriver_dead_thread() {
	dead = new DEAD();
	ut_a(dead != nullptr);
	
	while (srv_shutdown_state.load() == SRV_SHUTDOWN_NONE) {
		
		/* Every N sec (not exactly), it executes. */
		os_event_wait_time(srv_vdriver_dead_event, MICROSECS_IN_A_SECOND / 5000);

		/* If the system is shutdown, break the loop. */
		if (srv_shutdown_state.load() != SRV_SHUTDOWN_NONE) {
			break;
		}
		
		/* Memory barrier */
		os_rmb;

		/* Copy current view list from trx_sys */
		trx_sys->mvcc->clone_active_view_list(dead->get_view_list(), dead->get_length());

		/* Update dead zone */
		dead->update_dead_zone();

	}
	
	delete (dead);
}

void vdriver_stats_thread() {
	stat_sys = new STAT();

	while (srv_shutdown_state.load() == SRV_SHUTDOWN_NONE) {
		
		/* Every N sec (not exactly), it executes. */
		os_event_wait_time(srv_vdriver_stats_event, 2*MICROSECS_IN_A_SECOND);

		/* If the system is shutdown, break the loop. */
		if (srv_shutdown_state.load() != SRV_SHUTDOWN_NONE) {
			break;
		}
		/* Memory barrier */
		os_rmb;

  }
#ifdef HYU_PERF
	if (stat_sys->total_insert > 0) { 
			ib::warn() << "[HYU_PERF][FH] " 
					<< 100 * ((double)stat_sys->first_pr_cluster[VC_HOT] / (double)stat_sys->total_insert);
        
			ib::warn() << "[HYU_PERF][SH] " 
					<< 100 * ((double)stat_sys->second_pr_cluster[VC_HOT] / (double)stat_sys->total_insert);
         
			ib::warn() << "[HYU_PERF][NH] " 
					<< 100 * ((double)(stat_sys->total_ver_cluster[VC_HOT] - stat_sys->first_pr_cluster[VC_HOT] - stat_sys->second_pr_cluster[VC_HOT]) / (double)stat_sys->total_insert);
         
			ib::warn() << "[HYU_PERF][FC] " 
					<< 100 * ((double)stat_sys->first_pr_cluster[VC_COLD] / (double)stat_sys->total_insert);
		
			ib::warn() << "[HYU_PERF][SC] " 
					<< 100 * ((double)stat_sys->second_pr_cluster[VC_COLD] / (double)stat_sys->total_insert);
         
			ib::warn() << "[HYU_PERF][NC] " 
					<< 100 *((double)(stat_sys->total_ver_cluster[VC_COLD] - stat_sys->first_pr_cluster[VC_COLD] - stat_sys->second_pr_cluster[VC_COLD]) / (double)stat_sys->total_insert);
         
			ib::warn() << "[HYU_PERF][FL] " 
					<< 100 *((double)stat_sys->first_pr_cluster[VC_LLT] / (double)stat_sys->total_insert);

			ib::warn() << "[HYU_PERF][SL] " 
					<< 100 * ((double)stat_sys->second_pr_cluster[VC_LLT] / (double)stat_sys->total_insert);
          
			ib::warn() << "[HYU_PERF][NL] " 
					<< 100 * ((double)(stat_sys->total_ver_cluster[VC_LLT] - stat_sys->first_pr_cluster[VC_LLT] - stat_sys->second_pr_cluster[VC_LLT]) / (double)stat_sys->total_insert);
	}
#endif /* HYU_PERF */
	delete (stat_sys);
}


#endif /* HYU */

