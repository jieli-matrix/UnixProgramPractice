# Implementation to "Understanding Unix/Linux Programming"

操作系统是计算机最重要的系统软件，Unix操作系统历经迭代，至今仍是主流的操作系统。通过实现Unix中系统命令，进而深入了解系统组成与架构是一种有趣且深刻的学习方式。本项目在Ubuntu-20.04-LTS的基础上，对重要的Unix系统命令进行实现（涉及文件读写、文件系统、连接控制、信号机制、进程通信、Socket、线程并发等）。

参考教材：[Unix/Linux编程实践教程](https://book.douban.com/subject/1219329/)

## 目录

1. 文件读写
    - [more](./src/more)
    - [who](./src/who)
    - [ls](./src/ls)
    - [cp](./src/cp)


2. 文件系统
    - [pwd](./src/pwd)

3. 连接控制

4. 信号机制

5. 进程

6. Socket 

7. 线程并发

## 彩蛋

由于我对性能优化有兴趣，因此单独开篇分析常见性能优化技巧。

- [缓存优化](./src/optim/who)