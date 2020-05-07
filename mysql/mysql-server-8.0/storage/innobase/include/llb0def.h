#ifndef __DEF_H__
#define __DEF_H__

// Number of cutter threads (NOT AVAILABLE NOW)
#define SRV_THREADS 4

// Initialize parameter

// Length of pre-allocated vdesc list in the first time
#define VDESC_ALLOC_INIT 500

// Length of pre-allocated segment pointer array in the first time
#define NUM_SEG 100000

// Threshold about pre-allocating vdesc list in addition ( background thread )
#define VDESC_THRES 250

// Length of pre-allocating vdesc list in addition ( background thread )
#define VDESC_ALLOC 250

#define MICROSECS_IN_A_SECOND (1000000LL)
#define DECL_RET int ret = 0

// Mask For Version Locator
// U 4 bytes : segment id
// D 4 bytes : offset

#define SEG_MASK 0xffffffff00000000LL
#define OFF_MASK 0x00000000ffffffffLL

#define SEG_ID(v) (uint32_t)(v >> 32)

#define SEGMENT(v) (v & SEG_MASK)
#define OFFSET(v) (v & OFF_MASK)

// Fix up itself ( cutter )
#define F_DIRECT (true)

// Delegate to "update worker" to fix up
#define F_INDIRECT (false)

#define MEMORY_BARRIER asm volatile("" ::: "memory")


// segment variable mask

#define DEL_MARK (int)0x80000000

#define REF_MARK (int)0x7fffffff

#define SKIP_MARK (int)0x80000001

#endif /* __DEF_H__ */
