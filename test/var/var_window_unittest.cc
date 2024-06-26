//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#include <pthread.h>
#include <cstddef>
#include <memory>
#include <list>
#include <iostream>
#include <sstream>
#include <melon/utility/time.h>
#include <melon/utility/macros.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <melon/var/var.h>
#include <melon/var/window.h>

class WindowTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

TEST_F(WindowTest, window) {
    const int window_size = 2;
    // test melon::var::Adder
    melon::var::Adder<int> adder;
    melon::var::Window<melon::var::Adder<int> > window_adder(&adder, window_size);
    melon::var::PerSecond<melon::var::Adder<int> > per_second_adder(&adder, window_size);
    melon::var::WindowEx<melon::var::Adder<int>, 2> window_ex_adder("window_ex_adder");
    melon::var::PerSecondEx<melon::var::Adder<int>, window_size> per_second_ex_adder("per_second_ex_adder");

    // test melon::var::Maxer
    melon::var::Maxer<int> maxer;
    melon::var::Window<melon::var::Maxer<int> > window_maxer(&maxer, window_size);
    melon::var::WindowEx<melon::var::Maxer<int>, window_size> window_ex_maxer;

    // test melon::var::Miner
    melon::var::Miner<int> miner;
    melon::var::Window<melon::var::Miner<int> > window_miner(&miner, window_size);
    melon::var::WindowEx<melon::var::Miner<int>, window_size> window_ex_miner;

    // test melon::var::IntRecorder
    melon::var::IntRecorder recorder;
    melon::var::Window<melon::var::IntRecorder> window_int_recorder(&recorder, window_size);
    melon::var::WindowEx<melon::var::IntRecorder, window_size> window_ex_int_recorder("window_ex_int_recorder");

    adder << 10;
    window_ex_adder << 10;
    per_second_ex_adder << 10;

    maxer << 10;
    window_ex_maxer << 10;
    miner << 10;
    window_ex_miner << 10;

    recorder << 10;
    window_ex_int_recorder << 10;

    sleep(1);
    adder << 2;
    window_ex_adder << 2;
    per_second_ex_adder << 2;

    maxer << 2;
    window_ex_maxer << 2;
    miner << 2;
    window_ex_miner << 2;

    recorder << 2;
    window_ex_int_recorder << 2;
    sleep(1);

    ASSERT_EQ(window_adder.get_value(), window_ex_adder.get_value());
    ASSERT_EQ(per_second_adder.get_value(), per_second_ex_adder.get_value());

    ASSERT_EQ(window_maxer.get_value(), window_ex_maxer.get_value());
    ASSERT_EQ(window_miner.get_value(), window_ex_miner.get_value());

    melon::var::Stat recorder_stat = window_int_recorder.get_value();
    melon::var::Stat window_ex_recorder_stat = window_ex_int_recorder.get_value();
    ASSERT_EQ(recorder_stat.get_average_int(), window_ex_recorder_stat.get_average_int());
    ASSERT_DOUBLE_EQ(recorder_stat.get_average_double(), window_ex_recorder_stat.get_average_double());
}
