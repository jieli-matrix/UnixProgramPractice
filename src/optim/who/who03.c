/*
 * who03.c - who with buffered reads
 * - surpresses empty records
 * - formats time nicely
 * - buffers input (using utmplib)
 * usage: gcc -o who03 who03.c dataloader.c
 * ./who03
*/
#include <stdio.h>
#include <utmp.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "dataloader.h"

#define SHOWHOST

void show_info(struct utmp* utbufp);
void showtime(const long int timeval);
int main(){
    struct utmp* utbufp;

    if (utmp_open(UTMP_FILE) == -1){
        perror(UTMP_FILE);
        exit(-1);
    }
    while ((utbufp = utmp_next()) != (struct utmp*)NULL)
    {
        show_info(utbufp);
    }
    utmp_close();
    return 0;   
}

void show_info(struct utmp* utbufp)
{
    if ( utbufp->ut_type != USER_PROCESS)
        return;
    printf("%-8.8s", utbufp->ut_user);
    printf(" ");
    printf("%-8.8s", utbufp->ut_line);
    printf(" ");
    showtime(utbufp->ut_tv.tv_sec);
    printf(" ");
#ifdef SHOWHOST
    printf("(%s)", utbufp->ut_host);
#endif
    printf("\n");
}

void showtime(const long int timeval)
{
    char *cp;
    cp = ctime(&timeval);
    printf("%12.12s", cp+4);
}