#!/usr/bin/env bash

#
# Copyright 2023 The titan-search Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Add 'syntax="proto2";' to the beginning of all proto files under 
# PWD(recursively) if the proto file does not have it.
for file in $(find . -name "*.proto"); do
    if grep -q 'syntax\s*=\s*"proto2";' $file; then
        echo "[already had] $file";
    else 
        sed -i '1s/^/syntax="proto2";\n/' $file
        echo "[replaced] $file"
    fi
done
