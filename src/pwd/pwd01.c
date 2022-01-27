#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void printpathto(ino_t this_inode);
void inum_to_name(ino_t this_inode, char* name, int buflen);
ino_t get_inode(char* fname);


int main(){
    printpathto(get_inode("."));
    putchar('\n');
    return 0;
}

void printpathto(ino_t this_inode)
/*
 *   print the file system tree from leaf node 'this_inode'
 *   recursive implementation 
*/
{
    char name[BUFSIZ];
    ino_t my_inode;
    if (get_inode("..") != this_inode){
        chdir("..");
        inum_to_name(this_inode, name, BUFSIZ);
        my_inode = get_inode(".");
        printpathto(my_inode);
        printf("/%s", name);
    }
}

void inum_to_name(ino_t this_inode, char* name, int buflen)
/*
 * find the link name of this_inode in current dir
 * loop for readdir
*/
{
    DIR *dir_ptr;
    struct dirent *direntp;
    dir_ptr = opendir(".");

    // error handle
    if (dir_ptr == NULL){
        perror(".");
        exit(1);
    }

    // loop and find name of this_inode
    while ((direntp = readdir(dir_ptr)) != NULL)
    {
        if (direntp->d_ino == this_inode)
        {
            strncpy(name, direntp->d_name, buflen);
            name[buflen-1]='\0';
            closedir(dir_ptr);
            return ;
        }
    }

    fprintf(stderr, "error looking for inum %ld\n", this_inode);
    exit(1);
}


ino_t get_inode(char* fname)
/*
    return inode of 'fname' by 'stat' system call
    fname: file name  
*/
{
    struct stat info;
    if (stat(fname, &info) == -1){
        fprintf(stderr, "Cannot stat");
        perror(fname);
        exit(1);
    }
    return info.st_ino;
}
