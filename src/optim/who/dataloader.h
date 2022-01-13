#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <utmp.h>
#include <stdlib.h>
#include <unistd.h>

#define BATCH 16
#define NULLUT ((struct utmp*)NULL)
#define UTSIZE (sizeof(struct utmp))

static struct utmp utmpbuf[BATCH]; // storage
static int  num_recs; // records read in each recycle
static int  cur_rec; //  the current record
static int  fd_utmp = -1;

int utmp_open(char* filename);
struct utmp* utmp_next();
int utmp_reload();
void utmp_close();