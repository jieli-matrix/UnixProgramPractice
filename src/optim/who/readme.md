# 通过缓存优化提升who性能

复制一个5MB大小的文件，不同的缓冲区对应的执行时间如下（`cp`源码实现见[cp01](../../cp/cp01.c)）

<center><img src="https://s2.loli.net/2022/01/13/aoqcWJCYx2jQ1PH.png"></center>

5MB文件所需要的最大缓冲区为`5*1024=5120`，因此在在缓冲区低于`4096`时，随着缓冲区的提升，执行时间相应降低；缓冲区超过`4096`后，执行时间几乎没有降低。
造成这一现象的根本原因在于系统调用`read`需要时间，频繁的系统调用会降低程序效果。

## 系统调用过程发生了什么？

<center><img src="https://s2.loli.net/2022/01/13/NqKPyv3XQa2YzD1.png"></center>

用户进程位于用户空间，内核位于系统空间，而磁盘只能被内核直接访问。程序要读取磁盘上的数据只能通过系统调用`read`，而`read`位于内核中；因此当执行系统调用`read`时，执行权会从用户代码转移到内核代码。

在执行内核代码时，CPU对应`sudo`模式，这对应一些特殊的堆栈和内存环境，且必须在系统调用发生时建立好；在系统调用结束后，CPU要切换到用户模式，再把堆栈和内存环境恢复成用户程序运行时的状态。

Unix系统设计中有`sudo`模式与用户模式。在`sudo`模式下，程序可以直接访问磁盘、终端、打印机等设备，可以访问全部内存空间；而用户模式下，程序不能直接访问设备，只能访问特定部分的内存空间。在普通程序的运行过程中，系统根据需要不断地在这两种模式间进行切换。

## 优化思路: 批量数据读取

根据上述分析，我们可以对[who](../../who)程序进行缓存优化，优化的关键在于减少系统调用与适当的程序内存空间进行平衡，因此我们可以：
- 声明适当的缓冲空间
- 将登录用户数据批量读入缓冲中
- 当批量用户信息显示后，再触发系统调用再次将用户数据读入缓冲中

这样的设计思路使我们联想到python中`dataloader`的设计，我们可以先读取`batch`大小的用户数据，通过`iter`读取特定用户信息，直到该批量的数据读取完成，再重新读入。

具体代码实现见[who03.c](./who03.c)。

核心代码

``` c
// in who03.c

while ((utbufp = utmp_next()) != (struct utmp*)NULL)
    {
        show_info(utbufp);
    }

// in dataloader.c
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
```

`utmp_next()`返回下一个`utmp`结构体，在其内部，当缓冲区数据读取完毕，则触发`utmp_reload()`读取下一批次。

测试一下
``` shell 
$ gcc -o who03 who03.c dataloader.c
$ ./who03
hopeful  pts/7    Jan 13 20:58 (202.120.234.206)
```

## 内核缓冲技术

缓冲技术不仅可用于程序优化，事实上内核也使用缓冲技术来提高对磁盘的访问速度。

<center><img src="https://s2.loli.net/2022/01/14/8bLErIdpxvCcKDZ.png" width=60%></center>

内核对磁盘上的数据块作缓冲，当用户进程要从磁盘上读数据时，内核一般不直接读磁盘，而是将内核缓冲区中的数据复制到进程的缓冲区中。当进程所要求的数据块不在内核缓冲区中，内核会把相应的数据块加入到请求数据列表中，然后把该进程挂起，接着为其他进程服务。
一段时间之后（很短），内核把相应的数据块从磁盘读到内核缓冲区，然后再把数据复制到进程的缓冲区中，最后唤醒被挂起的进程。

