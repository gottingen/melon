#!/bin/bash

set -x

gtest_version=v1.10.x
gtest_url=https://github.com/google/googletest/archive/${gtest_version}.zip
current_dir=`pwd`
install_root=${current_dir}/carbin
download_root=${install_root}/download
echo ${install_root}

mkdir -p ${download_root}
cd ${download_root}

wget ${gtest_url}
unzip ${gtest_version}.zip
cd googletest-1.10.x
cmake . -DCMAKE_INSTALL_PREFIX=${install_root}
make install


cd ${download_root}
gflags_url=https://github.com/gflags/gflags/archive/refs/heads/master.zip
wget ${gflags_url}
unzip master.zip
cd gflags-master
cmake .  -DCMAKE_INSTALL_PREFIX=${install_root}
make install