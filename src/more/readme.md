# 通过C接口实现more命令

## 关于more 

`more`是Unix下用于分页显示文本内容的命令，通过`man`查看其常见用法

``` shell
NAME
       more - file perusal filter for crt viewing

SYNOPSIS
       more [options] file...

DESCRIPTION
       more  is a filter for paging through text one screenful at a time.  This version is especially primi‐
       tive.  Users should realize that less(1) provides more(1) emulation plus extensive enhancements.

```

## 实现思路

`more`具有交互性，具体而言，通过`more fileA`显示`fileA`的一页内容后，用户可通过以下三种命令进行交互：  

`q`     按`q`退出more  
`space` 按空格显示下一页  
`CR`    按回车显示下一行

因此，`more`命令实现的核心逻辑即为

> 从文件/标准输入流中读取一行到缓存区，再将缓存区读至标准输出，反复执行  
> 当读取行数达到页面限制，等待用户命令  
> 继续读取下一页/读取下一行/直接退出

## 如何使用
[more01.c](more01.c)与[more02.c](more02.c)均为`more`命令的简易实现，编译与使用如下

``` shell
gcc more01.c -o more01
./more01 more01.c
```
需要注意的是，这个简易版本在输入用户指令后(包括`q`, ` `与`CR`)必须回车才能执行(用于`EOF`退出)，而系统的`more`指令则是接受指令后即刻响应。

## 问题分析

[more01.c](more01.c)为`more`的最简实现，其完成了实现思路中的基本功能；然而，如果我们尝试`ls /bin | ./more01`则发现其无法正常显示；原因在于`more01`从标准输入流中读取用户输入，因此无法区分需要显示的重定向输入与用户指令。为了解决这个问题，我们可以从`/dev/tty`设备文件中读取键盘与鼠标输入，作为用户指令传给`see_more`程序。
通过上述修正，得到[more02.c](more02.c)。然而，[more02.c](more02.c)仍然存在没有已显示文件百分比，`more?`上滚等问题，需要进一步解决。
