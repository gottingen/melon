# abel - c++ base library

abel is an open-source collection of c++ library code. it designed to use c++ smartly.

![abel](/docs/images/abel.png)

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

* [algorithm](/docs/en/algorithm.md)
* [atomic](/docs/en/atomic.md) 
* [container](/docs/en/container.md)
* [debugging](/docs/en/debugging.md)
* [digest](/docs/en/digest.md)
* [flags](/docs/en/flags.md)
* [strings](/docs/en/strings.md)


## examples