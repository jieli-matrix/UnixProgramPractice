# 通过系统调用实现cp命令

## 关于cp命令

`cp`能够复制文件，其典型用法是

``` shell
cp src-file tgt-file
```

如果`tgt-file`所指定的文件不存在，`cp`就创建这个文件，如果已经存在就覆盖，`tgt-file`与`src-file`相同。

## cp的实现思路

`cp`实现的思路相对简单：
- 1 创建`tgt-file`
- 2 打开`src-file`
- 3 声明缓冲数组`buffer`，不断从`src-file`读取内容到`buffer`，再将`buffer`写入到`tgt-file`，直到读取完成。

因此，需要了解创建文件和写文件的系统调用（在[who](../who)中，我们使用了读文件和关闭文件的系统调用）。

*创建文件*

``` c
/*
 * creat: create or rewrite a file
 * filename: name of the file
 * mode: privilege of visiting file(rwx), e.g. 0644
 * fd: file identifier 
*/
#include <fcntl.h>
int fd = creat(char * filename, mode_t mode);
```

*写文件*

``` c
/*
 * write: write the data in memory into file
 * fd: file identifier 
 * buf: buffer data
 * amt: the size you write
 * result: number of written size. -1 if occurs error
*/
#include <unistd.h>
ssize_t result = write(int fd, void * buf, size_t amt)
```

与读数据类似，写数据可能发生实际写入数据`result`比`amt`少的情形：有两个原因，一是系统对文件的最大尺寸有限制，二是磁盘空间接近满了。


## 具体实现

系统编程的代码大多短小精悍，其中最常见的一点是，在操作中融入错误处理，例如

``` c
while ((n_chars = read(in_fd, buf, BUFFERSIZE)) > 0)
    {
        if (write(out_fd, buf, n_chars) != n_chars)
            oops("Write error to ", argv[2]);
    }
```

如果让我写，可能会这样

```c
while(1) {
    n_chars = read(in_fd, buf, BUFFERSIZE);
    if (n_chars < 1 )
        break;
    else {
        result = write(out_fd, buf, n_chars);
        if (result != n_chars)
            oops("Write error to ", argv[2]);
    }
}
```
则会过于冗余，因此在执行特定操作时，可直接判断其返回值进行错误处理。

具体实现见[cp01.c](cp01.c)，使用方法

``` c
gcc cp01.c -o cp01
./cp01 cp01.c cp02.c
```

## 改变文件的当前位置

Unix每次打开一个文件都会保存一个指针，以记录文件的当前位置。

<center><img src="https://s2.loli.net/2022/01/14/nXZRVW1w2bvSqCY.png" width=60%></center>

当从文件读/写数据时，内核从指针所标明的地方开始，读/写取指定的字节，然后移动位置指针，指向下一个未被读/写的字节。指针与*文件描述符*相关联，而不是与文件关联，所以如果两个程序同时打开一个文件，此时有两个指针，两个程序对文件的读/写操作不会互相干扰。

系统调用`lseek`可以改变已打开文件的当前位置

``` c
/*
 * lseek: reposition read/write file offset
 * off_t oldpos = lseek(int fd, off_t dist, int base)
 * fd: file identifier
 * dist: move distance
 * base: SEEK_SET -> begin location of the file
         SEEK_CUR -> current location of the file
         SEEK_END -> end location of the file
 * oldpos: -1        if occurs error
           oldpos    pointer position before changes
*/
```

