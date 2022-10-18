melon
===

melon系列框架现代化的后台开发框架。包括c++， go， python，rust四种语言版本。旨在提供方便、稳定、高效的
后台服务框架，并在多种业务场景中是使用不同语言，能够无障碍互通的框架体系。另外melon系统框架不仅提供rpc通信能力
更是一款基础服务开发库。

# 背景

在服务端编程的领域中，通常情况下，c/c++编程是高性能的代名词。但是高性能的背后，编程难度高，调试困难，业务迭代速
度慢等问题在生产环境中尤为棘手。使用c++编程，归纳起来有几个核心的问题

* 异步编程复杂。
* 业务调用关系在异步框架下，代码复杂。
* 内存生命周期管理在异步环境下复杂，难以调试。
* 多线程竞态问题处理

部分业务的逻辑处理对性能要求并不高，同时，对业务迭代的时间要求比较苛刻，特别是在AI蓬勃发展的今天
python语言的应用尤为广泛，因此，python与c++语言的无缝对接尤为重要。

随着硬件的发展，服务器的cpu核数从单核到多核，甚至可达上百个核心，高效利用多核也是编程中的极大的难题，
通过一套框架，很好的将这些基础而重要的问题从业务层剥离开，能很好的帮助业务提高效能。

* python版本[melon-py](https://github.com/melon-rpc/melon-py)
* go版本[melon-go](https://github.com/melon-rpc/melon-go)
* c++版本[melon-cpp](https://github.com/melon-rpc/melon-cpp)


# 开始使用

* [编译安装](docs/cn/getting_started.md)

* 基础库
    * [base](docs/cn/base.md)
    * [string](docs/cn/string.md)
    * [filesystem](docs/cn/filesystem.md)
    * [bootstrap](docs/cn/bootstrap.md)
    * [container](docs/cn/container.md)
    * [memory](docs/cn/memory.md)
    * [time](docs/cn/time.md)
    * [fiber](docs/cn/fiber.md)
    * [future](docs/cn/future.md)
    * [log](docs/cn/log.md)
* rpc
    * server
    
    * client
    
* example