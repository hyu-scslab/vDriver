
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#include "llb0time.h"




#define INT64CONST(x)   (x##LL)
#define USECS_PER_SEC   INT64CONST(1000000)




/* timestamp : second * 1000000 + microsecond */
Timestamp get_timestamp()
{
	Timestamp ts;
	struct timeval tp;
	gettimeofday(&tp, NULL);
	ts = (Timestamp) tp.tv_sec * USECS_PER_SEC + tp.tv_usec;

	return ts;
}








