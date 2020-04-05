/*-------------------------------------------------------------------------
 *
 * vchain.h
 *	  definitions of functionality for managing version chain
 *
 *
 *
 * src/include/storage/vchain.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef VCHAIN_H
#define VCHAIN_H

#include "storage/vcluster.h"

extern Size VChainShmemSize(void);
extern void VChainInit(void);

extern bool VChainLookupLocator(Oid rel_node,
								PrimaryKey primary_key,
								Snapshot snapshot,
								VLocator **ret_locator);
extern void VChainAppendLocator(Oid rel_node,
								PrimaryKey primary_key,
								VLocator *locator);
extern void VChainFixUpOne(VLocator* mid);

#endif							/* VCHAIN_H */
