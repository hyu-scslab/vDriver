/*-------------------------------------------------------------------------
 *
 * dead_zone.h
 *	  dead zone
 *
 *
 *
 * src/include/storage/dead_zone.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef DEAD_ZONE_H
#define DEAD_ZONE_H

#include "c.h"
#include "utils/dsa.h"
#include "utils/snapmgr.h"
#include "utils/timestamp.h"

#include "storage/vcluster.h"
#include "storage/thread_table.h"

#define DEAD_ZONE_SIZE	(SNAPSHOT_SIZE)

#ifndef INTERVAL_UPDATE_DEADZONE
#define INTERVAL_UPDATE_DEADZONE	(1)
#endif


typedef struct {
	/* Lower bound */
	TransactionId		left;

	/* Upper bound */
	TransactionId		right;
} DeadZone;

/* dead zone descriptor */
/* Dead zone is protected by DeadZoneLock(rw lock). */
/* DeadZoneLock is defined in lwlocknames.txt */
typedef struct {
	DeadZone			dead_zones[DEAD_ZONE_SIZE];
	int					cnt;
} DeadZoneDesc;




extern DeadZoneDesc*	dead_zone_desc;

extern Size DeadZoneShmemSize(void);
extern void DeadZoneInit(void);
extern pid_t StartDeadZoneUpdater(void);

extern void SetDeadZone(void);
extern TransactionId GetDeadZone(void);
extern bool RecIsInDeadZone(TransactionId xmin,
						 TransactionId xmax);
extern bool SegIsInDeadZone(TransactionId xmin,
						 TransactionId xmax);
extern bool VersionChainIsInDeadZone(Oid rel_node,
                         PrimaryKey primary_key);


#endif							/* DEAD_ZONE_H */
