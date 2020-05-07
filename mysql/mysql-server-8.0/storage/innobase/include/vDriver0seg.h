#ifndef vDriver0seg_h
#define vDriver0seg_h

#include "univ.i"

#ifdef HYU

#include "trx0rec.h"
#include <sys/types.h>
#include "fsp0fsp.h"
#include "mach0data.h"
#include "mtr0log.h"

#include "read0read.h"
#include "trx0undo.h"
#include "dict0dict.h"
#include "fsp0sysspace.h"
#include "que0que.h"
#include "read0read.h"
#include "row0ext.h"
#include "row0mysql.h"
#include "row0row.h"
#include "row0upd.h"
#include "trx0purge.h"
#include "ut0mem.h"
#include "lock0lock.h"
#include "hash0hash.h"
#include "ha0ha.h"
#include <string.h>
#include <map>


/* define */
#define NUM_WORKER_THREAD 300 // number of worker threads in mysql

#define NUM_VSEG 100000 // total number of segments
#define VSEG_SIZE (1024 * 64) // size of one segment: 64KB

#define NUM_VCLUSTER 3 // total number of clusters

#define VC_HOT 0
#define VC_COLD 1
#define VC_LLT 2

#define MUL_HOT 50 // determine version of hot cluster
#define MUL_LLT 100 // determine version of cold/LLT cluster

#define W_AVG (0.9999)
#define W_NEW (0.0001)
#define TRX_W_AVG (0.9)
#define TRX_W_NEW (0.1)

#define NUM_VSEG_IDX 1024 // number of version segment indices

#define VCACHE_PAGE_SIZE (1024 * 8) // size of one vCache page: 8KiB

#define VCACHE_SIZE (1024 * 1024 * 128) // size of vCache: 256 MiB / 1 GiB

#define NUM_VSEG_PAGE (VSEG_SIZE / VCACHE_PAGE_SIZE) // number of pages in one vSegment

#define NUM_REC 100000 // number of record in table

#define VER_LEN 256

#define REC_LEN 206

#define V_MIN_OFFSET 206

#define V_MAX_OFFSET 214

#define NUM_VER_IN_PAGE (VCACHE_PAGE_SIZE) / VER_LEN

#define EXTRA_BITS 5

#define NUM_VER_IN_SEG (VSEG_SIZE) / VER_LEN

#define HASH_LOCKS 128 // number of hash table locks

#define VCACHE_PIN (0x8000000000000000)
#define VCACHE_REFCNT_MASK (0x7FFFFFFFFFFFFFFF)

/* Statistics */
#ifdef HYU_PERF
#define INCR_STAT(s, v)\
	(void)__sync_fetch_and_add(&s->v, 1)
#else
#define INCR_STAT(s, v)\
	do{}while(0)
#endif /* HYU_PERF */
/* Done */


/* typedef */
typedef ib_id_t vnum_t; 				// version number

typedef ib_uint32_t vseg_id_t; 			// version segment id

typedef ib_id_t ver_key_t;  			// version locater key: 4byte segment_id + 4byte offset

typedef ib_uint32_t vpage_id_t; 		// page id

typedef ib_mutex_t VCluMutex; 			// mutex of vSegment descriptor in vCluster descriptor 

typedef ib_mutex_t VCacheLRUMutex; 		// vCache LRU mutex

typedef ib_mutex_t VCacheFreeListMutex; // vCache free page list mutex



// vCache_page_t: vCache page structure
typedef struct vCache_page {
	ib_uint64_t refcnt; // first bit will be used as pin
	bool is_dirty;
	vseg_id_t vseg_id; // vSegment id
	vpage_id_t vpage_id; // page id of a vSegment
	int cluster;
	int init;

	ib_uint32_t written_bytes; // written bytes of one page
	UT_LIST_NODE_T(struct vCache_page) LRU;
	UT_LIST_NODE_T(struct vCache_page) free_list;

	byte *buffer; // real data in here
}vCache_page_t;

// vCache_sys_t
typedef struct vCache_sys {
	UT_LIST_BASE_NODE_T(vCache_page_t) LRU; // eviction algorithm: LRU
	UT_LIST_BASE_NODE_T(vCache_page_t) free_list;
	VCacheLRUMutex LRU_mutex;
	VCacheFreeListMutex free_list_mutex;

	// hash table of vCache to access vpages
	hash_table_t  *page_hash;

}vCache_sys_t;

// vVersion_t: version structure that store to clusters
typedef struct vVersion {
	//ib_uint64_t version;
	byte version[256]; // record
	int length; // record length
	const byte *primary_key; // primary key of record
	ulint pk_len; // primary key length
}vVersion_t;

typedef struct vDriver_delta {
	vnum_t vlife;
	vnum_t tlife;
}vDriver_delta_t;

void vDriver_init();
void vDriver_shutdown();
vseg_id_t vDriver_add_segment(uint8_t cluster);
bool vDriver_cache_allot(vseg_id_t segnum, vpage_id_t vpage_id, ib_uint32_t page_num);
vCache_page_t* vDriver_init_page(vCache_page_t *page, ib_uint64_t refcnt, bool is_dirty, vseg_id_t vseg_id, vpage_id_t vpage_id, ib_uint32_t written_bytes, int init);
bool vDriver_write_dirty(vCache_page_t *page);
bool vDriver_free_page(vCache_page_t *page);
vCache_page_t* vDriver_evict_page();
vCache_page_t* vDriver_get_page(ver_key_t key);
int vDriver_insert_ver(vnum_t v_start, vnum_t v_end, vVersion *version, ib_uint32_t space_id);
vDriver_delta_t vDriver_update_delta(vDriver_delta_t temp);

extern ib_uint64_t delta_LLT;

/* DEFINE FOR DEAD ZONE */
#define NUM_DEADZONE (50)
#define LEN_DEAD_LIST (10)


/**
struct deadzone 

This one represents "DEADZONE".
For now, I limit the number of deadzone (NUM_DEADZONE)

range [ 2 * NUM_DEADZONE] : Each deadzone contains v_min & v_max.
len 					  : Length of deadzone
ref_cnt					  : Contention between deadzone updater & reader ( pruning & llb cutter )

Deadzone structure is managed by list (m_dead_list) whose maximum length is fixed (LEN_DEAD_LIST)

*/

struct deadzone {
	deadzone(): len(0), ref_cnt(0), oldest_active_trx_list_len(0), oldest_low_limit_id(0) {
		memset(range, 0x00, sizeof(uint64_t) * NUM_DEADZONE * 2);
		memset(oldest_active_trx_list, 0x00, sizeof(oldest_active_trx_list));
	}

	uint64_t range[2 * NUM_DEADZONE];

	uint64_t len;
	
	uint64_t ref_cnt;

	uint64_t oldest_active_trx_list[NUM_WORKER_THREAD];
	int oldest_active_trx_list_len;
	uint64_t oldest_low_limit_id;

	typedef UT_LIST_NODE_T(deadzone) node_t;
	node_t m_dead_list;
};

/**
class DEAD

This class represents "deadzone system".
*/
class DEAD {
		
	public:
		explicit DEAD();
		~DEAD();
	
		/* Get current view list */
        ReadView* get_view_list();
        
		/* Get length pointer */
		ulint* get_length();

		/* Update old deadzone to new one */
		void update_dead_zone();

		/* Check whether prune or not */
		bool can_pruning(vnum_t, vnum_t);
		
		/* Get current deadzone for reader (pruning & llb cutter) */
		deadzone* get_cur_zone() { return cur_zone; }

		/* Get latch(rwlock) */
		rw_lock_t* get_latch() { return &latch; }

		/* API for rwlock */
		void rw_x_lock(){ rw_lock_x_lock(&latch); }
		void rw_s_lock(){ rw_lock_s_lock(&latch); }
		void rw_x_unlock(){ rw_lock_x_unlock(&latch); }
		void rw_s_unlock(){ rw_lock_s_unlock(&latch); }

    private:
		/* Prevent copying */
		DEAD(const DEAD&);
		DEAD &operator=(const MVCC &);

	private:
		/* Get free dead node from list */
		deadzone *get_dead();
	
	private:
		/* latch for rwlock */
		rw_lock_t latch;

		/* view list for copying current one from trx_sys */
		ReadView* view_list;

		/* The length of view list */
        ulint length;

		/* Current deadzone pointer */
		deadzone* cur_zone;

	private:
		typedef UT_LIST_BASE_NODE_T(deadzone) dead_list_t;

		/** Free node ready for reuse */
		dead_list_t m_free;

		/** Active deadzone node */
		dead_list_t m_dead;
};

/**
class STAT

This class is used to statistical analysis. 
TODO :: " MUST FIX IT "
*/
class STAT {

	public:
		STAT();
		~STAT();
		
	public:
		// FIXME : Make setter & getter not now .
		/* # of 1st pruning */
		uint64_t first_pruning;
		
		/* # of 2nd pruning */
		uint64_t second_pruning;
		
		/* # of total vdriver insert */
		uint64_t total_insert;
		
		/* # of writing page to disk in page eviction */
		uint64_t total_page_write;

		/* # of removing file */
		uint64_t total_file_cut;

		/* FIXME !! # of pruning per deadzone*/
		uint64_t zone_lev1;
		uint64_t zone_lev2;

		/* number of segment in cluster */
		int num_seg[3];
		uint64_t first_pr_cluster[3];
		uint64_t second_pr_cluster[3];
		uint64_t total_ver_cluster[3];
};



/* Read specific version record from vDriver */
int vDriver_read_ver(ReadView*, const byte*, ulint, rec_t **, my_thread_id, dict_index_t*);

/* Check removability for HYU purger */
int vDriver_check_removability(const byte*, ulint, trx_id_t, trx_id_t, my_thread_id, dict_index_t*);

/* Read specific page from disk */
bool vDriver_read_page(vCache_page_t*);

/* Get free page in vCache after eviction */
vCache_page_t* vDriver_get_page_slow(ver_key_t, vseg_id_t, vpage_id_t);

/* Get record and return it */
rec_t* vDriver_get_record(vCache_page_t*, uint32_t);



/* These thread functions are used in background thread in innodb */

/* llb cutter thread function */
void vdriver_cut_thread();

/* updating deadzone thread function */
void vdriver_dead_thread();

/* statistics thread function */
void vdriver_stats_thread();

void vDriver_debug();




#endif /* HYU */
#endif
