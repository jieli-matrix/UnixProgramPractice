/*
 * who01.c 
 * open, read UTMP file and show results
 * bugs: couldn't convert time right
*/
#include <stdio.h>
#include <utmp.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define SHOWHOST

void show_info(struct utmp* utbufp);

int main(){
    struct utmp current_record; // read info into utmp struct
    int utmpfd; // open utmp file with file identifier
    int reclen = sizeof(current_record);

    if ((utmpfd = open(UTMP_FILE, O_RDONLY)) == -1){
        perror(UTMP_FILE);
        exit(1);        
    }
    while (read(utmpfd, &current_record, reclen) == reclen)
    {
        show_info(&current_record);
    }
    close(utmpfd);
    return 0;   
}

void show_info(struct utmp* utbufp)
{
    printf("%-8.8s", utbufp->ut_user);
    printf(" ");
    printf("%-8.8s", utbufp->ut_line);
    printf(" ");
    printf("%10d", utbufp->ut_tv.tv_sec);
    printf(" ");
#ifdef SHOWHOST
    printf("(%s)", utbufp->ut_host);
#endif
    printf("\n");
}

