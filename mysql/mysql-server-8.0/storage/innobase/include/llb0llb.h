#ifndef llb0llb_h
#define llb0llb_h

/****
  * Mimic DB version of LLB (Location Lookaside Buffer)
  * Now current LLB design is on-going.
  *
  * We assume these things.
  * 
  * 1. Every records are "HOT" and fixed size.
  *
  * 2. Every segment indices have the fixed number of records' version data.
  * 
  * 3. In look-up, Current one is to find version from tail-pointer (per-rec hash table)
  *
****/
#include <llb0include.h>
#include <llb0vdesc.h>

#include <vector>


class LLB {
	public:

		/* LLB System API */
		LLB(int num_versions, int table_size, uint64_t thread_table_size);
		~LLB();

		/* LLB API */
		void insert(SegNodePtr, VersionLoc, Xid, Xid, int, int);
		VersionLoc lookup(SegNodePtr, Tid, ReadView*, dict_index_t*);
    int check(SegNodePtr, Tid, dict_index_t*, uint64_t*, uint64_t);
		void cut(uint64_t*, uint64_t);
		void add_segment(Sid, int);

		VDescNodePtr get_tail_ptr(int clu_no) { return (vdesc_sys.get_tail_ptr(clu_no)); }

    uint64_t total_seg_cut_hi() { return (vdesc_sys.total_seg_cut_hi()); }
    uint64_t total_seg_cut_lo() { return (vdesc_sys.total_seg_cut_lo()); }
		uint64_t total_file_cut() { return (vdesc_sys.total_file_cut()); }

	private:

		VDesc_sys vdesc_sys;

		/* the number of records' version in segment index */
		int num_versions;

		/* number of records */
		int table_size;

		typedef enum llb_status { LLB_LIVE, LLB_EXIT } LLBStatus;

		// LLB Status
		LLBStatus llb_status;

};




#endif /* llb0llb_h */
