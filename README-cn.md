# abel - c++ base library

abel 取名来自 all basic experienced library,目标是将我们经过生产环境验证，具有高度向正确性和可用性的常用基础库，统一到一起，供业务使用。
经过业务的不断迭代，逐步完善abel， 如folly之于facebook。

![abel](https://github.com/gottingen/abel/blob/master/docs/images/abel.png)

## status

[![Build Status](https://www.travis-ci.org/gottingen/abel.svg?branch=master)](https://travis-ci.org/gottingen/abel)
 
项目| 数量
:--- | :---
测试组| 197
测试case | 2592

## 平台与编译器兼容

platform|compiler | status
:--- | :---| :---
centos6 | gcc-4.8.5 | ok
centos6 | gcc-5.3 | ok
centos7 | gcc-4.8.5 | ok
mac os  | llvm    | ok




## 内容索引

* [关于abel](#about)
* [编译abel](#build)
* [abel模块](#modules)
* [使用示例](#examples)
* [文章资源](#papers)
* [话题](#topics)

<a name="about"> </a>

## 关于abel

一直计划将我们使用的基础的库打造也有几年时间了。2020年1月1日。经过漫长的准备，abel来到这个世界，经历了疫情期只能封闭在家，难得有时间全力完善
它。经历了3个月的完善，今天第一次它第一次和大家见面。
abel 设计目标：
* 经过设计精良，经过完整测试的算法，容器的公共库。
* 高度模块化，尽量减少互相之间的依赖引用。
* 尽量做到跨平台，如linux， mac os， windows等平台
* 在任何平台和编译器上都做到0警告和错误。
* 发布的接口需要有完整的文档或者示例。
* 尽量减少库的体积。


<a name="build"> </a>

## 编译abel

abel目前使用cmake作为构建系统。

### 编译器要求

    - clang version > 3.3
    - gcc version > 4.8.5
    - cmake version > 3.5
### 编译前准备

在 centos6 系统上，系统默认的编译器gcc 4.4.7，可以安装devtoolset 解决编译器问题。例如
    
    > sudo yum install centos-release-scl
    > sudo yum install devtoolset-7
    > scl enable devtoolset-7 bash
    
    > gcc --version
    gcc (GCC) 7.2.1 20170829 (Red Hat 7.2.1-1)
    Copyright (C) 2017 Free Software Foundation, Inc.
    This is free software; see the source for copying conditions.  There is NO
    warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    
### 编译步骤

    > git clone git@v.src.corp.qihoo.net:cloudldd/abel.git
    > cd abel
    > make build [Debug|Release]

### 运行单元测试：
    
    > make test

### 生成软件包（centos)
   
redhat 系列支持rpm包格式会生成tgz以及rpm两种格式安装包，如abel-1.0.3-el6-x86_64-debug.rpm
位于 build/package目录下

    > git clone git@v.src.corp.qihoo.net:cloudldd/abel.git
    > cd abel
    > make package
    > ls build/package
    abel-1.0.3-el6-x86_64-debug.rpm

    
    
<a name="modules"> </a>

## abel模块

* [base](/docs/en/base.md) 模块是abel最基础的模块，跨平台宏，已经多版本编译器适配在这个模块中定义。如ABEL_COMPILER_VERSION,
ABEL_LIKELY等宏。

* [asl]() asl 是assist stl, abel的基准编译级别是c++11, 为了在c++11的标准下提供性能更好，实现更简洁的方法，asl模块提供若干兼容c++14，
 c++17等实现。
    * string_view(c++17)
    * filesystem(c++17)
    * variant(c++17)
    * optional(c++17)
    * span(c++17)
    * type_traits(部分c++20) c++11部分traits没有完整实现，并做了替换，具体可参见type_traits文件注释。compatible表示兼容实现，
    replace表示替换实现
    * std::function高效实现（apply与tuple结合使用）避免内存竞态问题
    * hash提供统一hash框架，内部采用cityhash
    * random，提供多种分布random算法，如卡方分布，伯努利分布，高斯分布等随机数生成器
    * 容器类，提供在不同应用京更加高效的容器实现，如intrusive_list, node_hash_map, 
    * 格式化类，提供比std::printf安全的printf函数，并且性能优化50%
* chrono 时间类库
* config 设计如一个方便的配置模块，兼容gflag使用方法。
* strings 字符库包括字符串操作如trim，cat等，并在性能优化。
* metric 兼容prometheus的监控类。
* digest md5/sha1等生成器。

<a name="examples"> </a>

## 使用示例

使用示例目前可以参阅test目录下单元测试的使用。

<a name="papers"> </a>

## 文章资源

* [Working Draft N4687](/docs/documnet/n4687.pdf)

<a name="topic"> </a>

## 话题

* [memory](/docs/en/topic/memory.md) 
* [concurrent](/docs/en/topic/concurrent.md)