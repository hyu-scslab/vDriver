#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>

// Transaction Id, Version start, Version end type
typedef uint64_t Xid;

// Thread Id
typedef uint64_t Tid;

// Key type
typedef int64_t Key;

// Version Locator type
typedef uint64_t VersionLoc;

// Segment Id
typedef uint32_t Sid;

// Locator
typedef uint32_t Loc;

// Timestamp
typedef uint64_t Timestamp;

// Thread Id
typedef uint64_t ThreadId;

using namespace std;

#endif /* __TYPES_H__ */
