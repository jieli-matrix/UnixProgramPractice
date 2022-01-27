# 通过系统调用实现pwd命令

## 关于pwd命令

`pwd`能够显示当前路径，其典型用法是

``` shell
$ pwd
/home/hopeful/UnixProgramPractice/src
```

在介绍`pwd`之前，需要对Unix文件系统进行介绍，包括文件系统树、目录结构、文件系统构造与链接。

## 文件系统

从用户角度来看，Unix系统中硬盘的文件组成一棵文件树，每个目录能包含文件或其他目录。以本项目为例，列出文件树如下

``` shell
hopeful@VM-0-7-ubuntu:~/UnixProgramPractice$ tree
.
├── LICENSE
├── ReadME.md
└── src
    ├── cp
    │   ├── cp01.c
    │   └── readme.md
    ├── ls
    │   ├── ls01
    │   ├── ls01.c
    │   └── readme.md
    ├── more
    │   ├── more01.c
    │   ├── more02.c
    │   └── readme.md
    ├── optim
    │   └── who
    │       ├── dataloader.c
    │       ├── dataloader.h
    │       ├── readme.md
    │       └── who03.c
    ├── pwd
    │   ├── pwd01
    │   ├── pwd01.c
    │   └── readme.md
    └── who
        ├── readme.md
        ├── who01.c
        └── who02.c

8 directories, 20 files
```

对于文件系统，核心对象是**文件**和**目录**，尽管在Unix下“一切都是文件”，但此处我们仍强调**目录**是特殊的文件——*目录是文件的列表*。围绕文件和目录，我们介绍相关的操作。

### 文件操作命令

- cp
- mv
- ln
- ls
- rm
- cat

### 针对目录树命令

- ls -R  
选项`-R`要求列出目录及子目录的所有内容

``` shell
$ ls -R .
.:
LICENSE  ReadME.md  src

./src:
cp  ls  more  optim  pwd  who

./src/cp:
cp01.c  readme.md

./src/ls:
ls01  ls01.c  readme.md

./src/more:
more01.c  more02.c  readme.md

./src/optim:
who

./src/optim/who:
dataloader.c  dataloader.h  readme.md  who03.c

./src/pwd:
pwd01  pwd01.c  readme.md

./src/who:
readme.md  who01.c  who02.c
```

- chmod - R  
`-R`可修改子目录中所有文件的许可权限。
- du  
`disk usage`的缩写，给出指定目录及其子目录下所有文件占用硬盘中数据块的总数。(*通常，一个数据块大小为512KB*)
- find
寻找符合要求的文件

### 文件系统内部实现

这一节从系统的角度出发，考虑如何从磁盘构建一个文件系统。这一过程有三层抽象

- Level 1：从磁盘到分区  
每个分区可视为独立的磁盘。
- Level 2: 从磁盘到块序列  
每个盘面可划分为多个磁道，划分磁道弧段为扇区——扇区，*又称为数据块*，为磁盘上的基本存储单元，每个扇区有512字节。一个将磁盘扇区编号的系统使我们可以将磁盘视为一系列块的组合。

<div align="center"><img src ="https://upload-images.jianshu.io/upload_images/2843224-46fb935cd31addbd.png?imageMogr2/auto-orient/strip|imageView2/2/w/690/format/webp" width=60%></div>  

- Level 3: 从块序列到三个区域的划分  

文件系统可用于存储  
    - 文件内容  
    - 文件属性(文件所有者、日期等)  
    - 目录(文件名的列表)  

Unix系统将磁盘块分成了:超级块、i-节点表和数据区，以存储上述三种不同类型的数据。  

<div align ="center"><img src = "https://s2.loli.net/2022/01/26/owdUDESZvmCOFi3.png" width=70%></div>

(1) 超级块  
该块用于存放文件系统本身的结构信息，包括文件系统大小和状态、标号、柱面组等。  

(2) i-节点表  
该块存放有关文件的详细信息，每个文件均具有属性，如大小、文件所有者、最近修改/访问时间，这些性质被记录在i-节点结构中。一般而言，一个inode为128字节，i-节点表是inode的列表。  

(3) 数据区  
该块存放每个文件的内容。磁盘上所有块的大小是一样的（例如，512KB），如果文件包含了超过一个块的内容，那么会被存放在多个磁盘块中。需要说明的是，目录是一种*特殊的文件*，其内容为文件名的列表。  

关于文件系统内部实现更多内容，可见[Oracle文档](https://docs.oracle.com/cd/E19253-01/819-7062/6n91k1fuj/index.html)。

### 目录

《数据结构》告诉我们，树是递归定义的抽象数学概念，其实现可通过父、子指针的链表。类似，用户所看到的目录树，其系统实现则是含指针的列表，接下来介绍目录树的系统视图。

![dirtree.png](https://s2.loli.net/2022/01/26/clqW6smf2AMHeEr.png)

对于每个目录而言，其必然有两项

- 当前目录 `.`
- 父级目录 `..`

*Note: 这也解释了为什么任一目录的链接数>=2*  
通过这种方式，我们能够从文件树的任一节点，通过回溯父级目录的方式走到根目录（根目录的判定条件为`inode(".") == inode("..")`）。这是编写`pwd`程序的基础。  
接下来我们回答三个问题，以阐明“目录是文件名的列表”这一观点

(1) “文件在目录中”的含义  
目录包含的是文件的引用，每个引用称为链接，文件的内容存储在数据块，文件的属性被记录在inode中，inode节点的编号和文件引用存储在目录中。  

以“文件`x`在目录`a`中”意味着在目录`a`中有一个指向编号为402的inode，其引用名为x。再次说明，文件内容存储在数据区，没有“文件名”，只有引用名（硬链接与符号链接）。  

(2) “目录包含子目录”的含义
与(1)类似，当前目录包含(inode编号，子目录引用)这一项。这并未矛盾，因为目录是一种特殊的文件，因此也具有inode编号。

(3) 多重链接及链接数

在`demodir`目录树中，inode402有两个链接，一个在目录`a`中，称为`x`；另一个在目录`d1`中，称为`xlink`。在文件系统中，并没有原始文件的概念（与创建时间无关），这两个链接状态完全相同，称为指向文件的硬链接。


### 文件系统的工作原理

接下来，从系统角度分析创建文件、查看文件时，文件系统是如何工作的。
#### 创建文件

考虑以下命令，该命令创建了`userlist`文件，并写入了当前系统的用户信息作为文件内容。

``` shell
who > userlist
```

- 1 分配inode  

文件属性的存储：内核首先找到空闲inode，编号47，将文件信息、属性记录在inode中。  

- 2 分配数据块  

文件内容的存储：新文件需要3个存储块，内核从空闲数据块中找到3个数据块，编号为200, 627和992。内核依次将文件内容复制到对应数据块中。  

- 3 更新inode  

根据*分配数据块*，更新inode47的数据块列表。  

- 4 更新目录  

当前可视为(inode, file_reference)的列表，当创建userlist文件后，应在当前目录增加这一记录(47, userlist)。

![creatnew.png](https://s2.loli.net/2022/01/27/hlTapOoQAgtqjub.png)


#### cat命令

考虑以下命令

``` shell
cat userlist
```

- 1 定位inode  

在当前目录中检索含userlist的项，并定位相应的inode编号为47。  

- 2 读取inode内容  

在inode节点表中定位inode 47，并读取其相应的结构信息，得知文件数据块为200, 627, 992。  

- 3 访问数据块  

不断调用`read`函数，使内核不断将字节从磁盘复制到内核缓冲区，进而到达用户空间。

![cat.png](https://s2.loli.net/2022/01/27/2fqyVRYU5tnojCD.png)  

## 相关系统调用

介绍目录树相关的命令及系统调用

(1) mkdir  

系统调用`mkdir`  
``` c
#include <sys/stat.h>
#include <sys/types.h>
int result = mkdir(char* pathname, mode_t mode);
```  

工作原理：(1)分配inode节点记录目录属性；(2)分配数据块，更新目录内容（'.'和'..'及其inode）；(3)更新父目录的目录内容(增加(inode, pathname))。

(2) rmdir

系统调用`rmdir`

```c
#include <unistd.h>
int result = rmdir(const char* path);
```
工作原理类似`mkdir`，由此也知道，`rmdir`仅限于删除空目录。

(3) rm

系统调用`unlink`

```c
#include <unistd.h>
int result = unlink(const char* path);
```

`rm`仅删除文件的链接，而非清除其在磁盘上的数据块内容；当链接数为0时，相应的inode与数据块自动释放。

(4) ln
创建文件链接，系统调用`ln`

```c
#include <unistd.h>
int result = link(const char* orig, const char* new);
```

(5) mv  

系统调用`rename`

```c
int result = rename(const char* from, const char* to);
```

`rename`的工作原理容易想到，其通过`link`复制链接至新路径，再通过`unlink`删除原有链接。


(6) cd  

系统调用`chdir`

``` c
#include <unistd.h>
int result = chdir(const char* path);
```

`cd`用于改变进程的当前目录，但并不影响目录文件。
在系统内部，进程有一个存放当前目录inode的变量，从一个目录进入另一个目录只是改变了该变量的值。

## pwd命令实现

考虑`pwd`的原理：
- 获取当前目录的inode编号('.')
- 回到父级目录，根据inode编号查询链接
- 递归执行，直到根目录

因此，需要实现以下三个函数

``` c
// 获取inode编号
ino_t get_inode(char* fname);
// 查询链接
void inum_to_name(ino_t this_inode, char* name, int buflen);
// 递归执行
void printpathto(ino_t this_inode);
```

具体实现见[pwd01.c](./pwd01.c),编译如下

``` shell
gcc pwd01.c -o pwd01
./pwd01
/home/hopeful/UnixProgramPractice/src/pwd
```
