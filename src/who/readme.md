# 通过系统调用实现who命令

## 关于who

`who`是Unix下用于查看系统活动用户情况的命令，通过`man`查看其用法以及其工作原理

``` shell
WHO(1)                                                                                                       User Commands                                                                                                       WHO(1)

NAME
       who - show who is logged on

SYNOPSIS
       who [OPTION]... [ FILE | ARG1 ARG2 ]

DESCRIPTION
       Print information about users who are currently logged in.
...
If FILE is not specified, use /var/run/utmp.  /var/log/wtmp as FILE is common.
```
 
通过分析，发现`who`命令通过读取`/var/run/utmp`文件，向用户显示系统已登陆用户的信息。然而，直接查看`/var/run/utmp`为乱码，原因是用户信息按utmp数据结构存储。查看`utmp`数据结构信息

``` shell
man 5 utmp

NAME
       utmp, wtmp - login records

SYNOPSIS
       #include <utmp.h>
DESCRIPTION
struct utmp {
               short   ut_type;              /* Type of record */
               pid_t   ut_pid;               /* PID of login process */
               char    ut_line[UT_LINESIZE]; /* Device name of tty - "/dev/" */
               char    ut_id[4];             /* Terminal name suffix,
                                                or inittab(5) ID */
               char    ut_user[UT_NAMESIZE]; /* Username */
               char    ut_host[UT_HOSTSIZE]; /* Hostname for remote login, or
                                                kernel version for run-level
                                                messages */
               struct  exit_status ut_exit;  /* Exit status of a process
                                                marked as DEAD_PROCESS; not
                                                used by Linux init (1 */
               /* The ut_session and ut_tv fields must be the same size when
                  compiled 32- and 64-bit.  This allows data files and shared
                  memory to be shared between 32- and 64-bit applications. */
           #if __WORDSIZE == 64 && defined __WORDSIZE_COMPAT32
               int32_t ut_session;           /* Session ID (getsid(2)),
                                                used for windowing */
               struct {
                   int32_t tv_sec;           /* Seconds */
                   int32_t tv_usec;          /* Microseconds */
               } ut_tv;                      /* Time entry was made */
           #else
                long   ut_session;           /* Session ID */
                struct timeval ut_tv;        /* Time entry was made */
           #endif

               int32_t ut_addr_v6[4];        /* Internet address of remote
                                                host; IPv4 address uses
                                                just ut_addr_v6[0] */
               char __unused[20];            /* Reserved for future use */
           };
```

## 实现思路

分析可知，我们在编写代码时，需要包含头文件`utmp.h`，我们最关心的信息是`ut_user`, `ut_line`, `ut_host`和`tv_sec`。

### 读写

与[more](../more)相比，`who`需要从文件中读取结构体到缓冲中，并最终输出到标准输出流。如果继续使用C接口，`fgets`与`getc`仅能按行或按字符读取，无法读取数据结构。这里，我们使用系统调用的`open`与`read`函数进行读取

``` c
/* 
 * open a file
 * name: file name
 * how: O_RDONLY, O_WRONLY, or ORDWR
 * return value: 
 *      -1  error
 *      int success 
*/
#include <fcntl.h>
int fd = open(char* name, int how)
```

`open`函数打开磁盘文件，通过文件描述符`fd`建立磁盘文件与用户进程之间的连接，该连接不仅指向打开的文件，同时有指向读取文件内容的指针(联系`lseek`)。接下来，我们将文件描述符`fd`传给系统调用`read`函数，以读取文件数据到缓冲区。

``` c
/*
 * read file into buffer
 * fd: file identifier
 * buf: buffer
 * qty: bytes you want to read
*/
#include <unistd.h>
ssize_t numread = read(int fd, void *buf, size_t qty)
```
注意，这里提供了`read`的返回值`numread`：可能出现`numread`与`qty`不等的情形，有时可能是文件中剩余的数据没有要求那么多，对这种情况我们需要进行相应的错误处理。

## 如何使用

实现可查看[who01.c](./who01.c)
``` shell
gcc who01.c -o who01
./who01
~/UnixProgramPractice/src/who$ ./who01
reboot   ~        1612407202 (5.4.0-42-generic)
LOGIN    tty1     1612407216 ()
LOGIN    ttyS0    1612407216 ()
runlevel ~        1612407237 (5.4.0-42-generic)
hopeful  pts/0    1619421869 ()
         pts/1    1619152897 ()
         pts/2    1612417321 ()
         pts/3    1641830570 ()
         pts/4    1612964580 ()
         pts/5    1616985877 ()
         pts/9    1612807953 ()
         pts/7    1616319232 ()
         pts/8    1613278347 ()
hopeful  pts/6    1641870135 (202.120.234.206)
```

与系统自带`who`比较

``` shell
hopeful@VM-0-7-ubuntu:~$ who
hopeful  pts/6        2022-01-11 11:02 (202.120.234.206)
```
发现没有正确显示时间，以及需要消除空白记录，仅保存用户记录。

### 迭代改进

迭代改进实现见[who02.c](./who02.c)，具体改进方法

- 通过`ut_type`判断是否为用户程序；如果是，则打印信息，反之跳过。
- 通过`ctime`将整数时间转换为可读时间。

#### 运行

``` shell
gcc who02.c -o who02
:~/UnixProgramPractice/src/who$ ./who02
hopeful  pts/6    Jan 11 11:02 (202.120.234.206)
```