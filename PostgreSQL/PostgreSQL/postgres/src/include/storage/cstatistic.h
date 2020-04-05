/*-------------------------------------------------------------------------
 *
 * cstatistic.h
 *	  statistics about vDriver & vanilla
 *
 *
 *
 * src/include/storage/cstatistic.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef CSTATISTIC_H
#define CSTATISTIC_H

#include "c.h"
#include "utils/dsa.h"
#include "utils/snapshot.h"
#include "utils/snapmgr.h"
#include "utils/timestamp.h"


/* vDriver statistics descriptor */
typedef struct {
    /* chain counter */
    int64_t         cnt_chain;
} CStatisticDesc;




extern CStatisticDesc*	cstatistic_desc;

extern uint64_t cnt_version_chain_vanilla;
extern uint64_t cnt_version_chain_vdriver;

extern Size CStatisticShmemSize(void);
extern void CStatisticInit(void);



#endif							/* CSTATISTIC_H */
