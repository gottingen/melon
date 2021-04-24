#!/bin/bash

set -x

current_dir=`pwd`
third_dir=${current_dir}/third

install_root=${current_dir}/carbin
download_root=${install_root}/download
echo ${install_root}

mkdir -p ${download_root}
cd ${download_root}
cp ${third_dir}/* .

unzip gtest.zip
cd googletest-1.10.x
cmake . -DCMAKE_INSTALL_PREFIX=${install_root}
make install


cd ${download_root}
unzip gflags.zip
cd gflags-master
cmake .  -DCMAKE_INSTALL_PREFIX=${install_root}
make install