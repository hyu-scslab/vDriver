/*-------------------------------------------------------------------------
 *
 * vpool.h
 *	  Memory pool definitions for version cluster
 *
 *
 *
 * src/include/storage/vpool.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef VPOOL_H
#define VPOOL_H

typedef struct VPool {
	char	*freelist;
	/* may need a padding */

	int		size_pool;
	int 	bytes_entry;
	char	*pool_ptr;
} VPool;

extern Size VPoolShmemSize(int size_pool, int bytes_entry);
extern void VPoolShmemInit(char *pool_name, int size_pool, int bytes_entry);

#endif							/* VPOOL_H */
