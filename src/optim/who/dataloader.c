#include "dataloader.h"

int utmp_open(char* filename){
    fd_utmp = open(filename, O_RDONLY);
    cur_rec = num_recs = 0;
    return fd_utmp;
}

struct utmp* utmp_next()
{
    struct utmp* recp;
    
    if (fd_utmp == -1)
        return NULLUT;
    if (cur_rec == num_recs && utmp_reload() == 0)
        return NULLUT;
    
    recp = (struct utmp*) &utmpbuf[cur_rec];
    cur_rec++;
    return recp;
}

int utmp_reload()
{
    int batch_read;
    batch_read = read(fd_utmp, utmpbuf, BATCH*UTSIZE);
    num_recs = batch_read / UTSIZE;
    cur_rec = 0;
    return num_recs;
}

void utmp_close()
{
    if (fd_utmp != -1)
        close(fd_utmp);
}