# abel - c++ base library

abel is an open-source collection of c++ library code. it designed to use c++ smartly.

![abel](https://github.com/gottingen/abel/blob/master/docs/images/abel.png)

## status

[![Build Status](https://www.travis-ci.org/gottingen/abel.svg?branch=master)](https://travis-ci.org/gottingen/abel)

## content index

* [about abel](#about)
* [about cmake](#cmake)
* [build abel](#build)
* [modules](#modules)
* [examples](#examples)

<a name="about"> </a>

## about abel

for several years age, I have plan to prepare start a repository to de-duplicate code from the projects I have write and
projects I like (open source). so I would develop iteratively over it. I have try it on many private project (although 
it on github).  fortunately， it comes on Jan 1st 2020, named it abel, the name of a Mathematician.

since the inception, its goal is to <font size=16> **`consolidate algorithms, data structures, system operations and make it under control`**</font>
the goals are:

* to have a lib be well implemented and tested tools and algorithm
* aim high modularity with as little dependencies between modules as possible.
* zero external dependencies
* build on all platform with c++, such as linux, mac, android, windows, mobiles
* no warning and bugs on any platform and compiler
* interface of published have full documentation， use case description，performance benchmark and evaluation
* make overhead down, small overall size


collect code and ideas from:

* [abseil-cpp](https://github.com/abseil/abseil-cpp) from google, and make abel basic framework
* [EASTL](https://github.com/electronicarts/EASTL) from EA
* fermat my private cpp lib


<a name="cmake"> </a>
 
## about cmake


<a name="build"> </a>

## build abel

abel use cmake as build system. sample to build abel

compiler requirement

    - clang version > 3.3
    - gcc version > 4.8
    - cmake version > 3.5(if you build benchmark) otherwise 2.8 is enough
build step

    $ git clone https://github.com/gottingen/abel.git
    $ cd abel
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ make test
    
<a name="modules"> </a>

## modules

<a name="examples"> </a>

* [base](/docs/en/base.md)

* [algorithm](/docs/en/algorithm.md)
* [atomic](/docs/en/atomic.md) 
* [container](/docs/en/container.md)
* [debugging](/docs/en/debugging.md)
* [digest](/docs/en/digest.md)
* [flags](/docs/en/flags.md)
* [strings](/docs/en/strings.md)


## examples