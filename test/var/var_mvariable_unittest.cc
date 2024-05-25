// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


// Date 2021/11/17 14:57:49

#include <pthread.h>                                // pthread_*
#include <cstddef>
#include <memory>
#include <iostream>
#include <set>
#include <string>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include "melon/utility/time.h"
#include "melon/utility/macros.h"
#include "melon/var/var.h"
#include "melon/var/multi_dimension.h"

static const int num_thread = 24;

static const int idc_count = 20;
static const int method_count = 20;
static const int status_count = 50;
static const int labels_count = idc_count * method_count * status_count;

static const std::list<std::string> labels = {"idc", "method", "status"};

struct thread_perf_data {
    melon::var::MVariable* mvar;
    melon::var::Variable*  rvar;
    melon::var::Variable*  wvar;
};

class MVariableTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {
    }
};

namespace foo {
namespace bar {
class Apple {};
class BaNaNa {};
class Car_Rot {};
class RPCTest {};
class HELLO {};
}
}

TEST_F(MVariableTest, expose) {
    std::vector<std::string> list_exposed_vars;
    std::list<std::string> labels_value1 {"bj", "get", "200"};
    melon::var::MultiDimension<melon::var::Adder<int> > my_madder1(labels);
    ASSERT_EQ(0UL, melon::var::MVariable::count_exposed());
    my_madder1.expose("request_count_madder");
    ASSERT_EQ(1UL, melon::var::MVariable::count_exposed());
    melon::var::Adder<int>* my_adder1 = my_madder1.get_stats(labels_value1);
    ASSERT_TRUE(my_adder1);
    ASSERT_STREQ("request_count_madder", my_madder1.name().c_str());

    ASSERT_EQ(0, my_madder1.expose("request_count_madder_another"));
    ASSERT_STREQ("request_count_madder_another", my_madder1.name().c_str());

    ASSERT_EQ(0, my_madder1.expose("request-count::madder"));
    ASSERT_STREQ("request_count_madder", my_madder1.name().c_str());

    ASSERT_EQ(0, my_madder1.expose("request.count-madder::BaNaNa"));
    ASSERT_STREQ("request_count_madder_ba_na_na", my_madder1.name().c_str());

    ASSERT_EQ(0, my_madder1.expose_as("foo::bar::Apple", "request"));
    ASSERT_STREQ("foo_bar_apple_request", my_madder1.name().c_str());

    ASSERT_EQ(0, my_madder1.expose_as("foo.bar::BaNaNa", "request"));
    ASSERT_STREQ("foo_bar_ba_na_na_request", my_madder1.name().c_str());

    ASSERT_EQ(0, my_madder1.expose_as("foo::bar.Car_Rot", "request"));
    ASSERT_STREQ("foo_bar_car_rot_request", my_madder1.name().c_str());

    ASSERT_EQ(0, my_madder1.expose_as("foo-bar-RPCTest", "request"));
    ASSERT_STREQ("foo_bar_rpctest_request", my_madder1.name().c_str());

    ASSERT_EQ(0, my_madder1.expose_as("foo-bar-HELLO", "request"));
    ASSERT_STREQ("foo_bar_hello_request", my_madder1.name().c_str());

    my_madder1.expose("request_count_madder");
    ASSERT_STREQ("request_count_madder", my_madder1.name().c_str());
    list_exposed_vars.push_back("request_count_madder");

    ASSERT_EQ(1UL, my_madder1.count_stats());
    ASSERT_EQ(1UL, melon::var::MVariable::count_exposed());

    std::list<std::string> labels2 {"user", "url", "cost"};
    melon::var::MultiDimension<melon::var::Adder<int> > my_madder2("client_url", labels2);
    ASSERT_EQ(2UL, melon::var::MVariable::count_exposed());
    list_exposed_vars.push_back("client_url");

    std::list<std::string> labels3 {"product", "system", "module"};
    melon::var::MultiDimension<melon::var::Adder<int> > my_madder3("request_from", labels3);
    list_exposed_vars.push_back("request_from");
    ASSERT_EQ(3UL, melon::var::MVariable::count_exposed());

    std::vector<std::string> exposed_vars;
    melon::var::MVariable::list_exposed(&exposed_vars);
    ASSERT_EQ(3, exposed_vars.size());

    my_madder3.hide();
    ASSERT_EQ(2UL, melon::var::MVariable::count_exposed());
    list_exposed_vars.pop_back();
    exposed_vars.clear();
    melon::var::MVariable::list_exposed(&exposed_vars);
    ASSERT_EQ(2, exposed_vars.size());
}

TEST_F(MVariableTest, labels) {
    std::list<std::string> labels_value1 {"bj", "get", "200"};
    melon::var::MultiDimension<melon::var::Adder<int> > my_madder1("request_count_madder", labels);

    ASSERT_EQ(labels.size(), my_madder1.count_labels());
    ASSERT_STREQ("request_count_madder", my_madder1.name().c_str());

    ASSERT_EQ(labels, my_madder1.labels());

    std::list<std::string> labels_too_long;
    std::list<std::string> labels_max;
    int labels_too_long_count = 15;
    for (int i = 0; i < labels_too_long_count; ++i) {
        std::ostringstream os;
        os << "label" << i;
        labels_too_long.push_back(os.str());
        if (i < 10) {
            labels_max.push_back(os.str());
        }
    }
    ASSERT_EQ(labels_too_long_count, labels_too_long.size());
    melon::var::MultiDimension<melon::var::Adder<int> > my_madder2("request_labels_too_long", labels_too_long);
    ASSERT_EQ(10, my_madder2.count_labels());
    ASSERT_EQ(labels_max, my_madder2.labels());
}

TEST_F(MVariableTest, dump) {
    std::string old_var_dump_interval;
    std::string old_mvar_dump;
    std::string old_mvar_dump_prefix;
    std::string old_mvar_dump_format;

    google::GetCommandLineOption("var_dump_interval", &old_var_dump_interval);
    google::GetCommandLineOption("mvar_dump", &old_mvar_dump);
    google::GetCommandLineOption("mvar_dump_prefix", &old_mvar_dump_prefix);
    google::GetCommandLineOption("mvar_dump_format", &old_mvar_dump_format);

    google::SetCommandLineOption("var_dump_interval", "1");
    google::SetCommandLineOption("mvar_dump", "true");
    google::SetCommandLineOption("mvar_dump_prefix", "my_mdump_prefix");
    google::SetCommandLineOption("mvar_dump_format", "common");

    melon::var::MultiDimension<melon::var::Adder<int> > my_madder("dump_adder", labels);
    std::list<std::string> labels_value1 {"gz", "post", "200"};
    melon::var::Adder<int>* adder1 = my_madder.get_stats(labels_value1);
    ASSERT_TRUE(adder1);
    *adder1 << 1 << 3 << 5;

    std::list<std::string> labels_value2 {"tc", "get", "200"};
    melon::var::Adder<int>* adder2 = my_madder.get_stats(labels_value2);
    ASSERT_TRUE(adder2);
    *adder2 << 2 << 4 << 6;

    std::list<std::string> labels_value3 {"jx", "post", "500"};
    melon::var::Adder<int>* adder3 = my_madder.get_stats(labels_value3);
    ASSERT_TRUE(adder3);
    *adder3 << 3 << 6 << 9;

    melon::var::MultiDimension<melon::var::Maxer<int> > my_mmaxer("dump_maxer", labels);
    melon::var::Maxer<int>* maxer1 = my_mmaxer.get_stats(labels_value1);
    ASSERT_TRUE(maxer1);
    *maxer1 << 3 << 1 << 5;

    melon::var::Maxer<int>* maxer2 = my_mmaxer.get_stats(labels_value2);
    ASSERT_TRUE(maxer2);
    *maxer2 << 2 << 6 << 4;

    melon::var::Maxer<int>* maxer3 = my_mmaxer.get_stats(labels_value3);
    ASSERT_TRUE(maxer3);
    *maxer3 << 9 << 6 << 3;

    melon::var::MultiDimension<melon::var::Miner<int> > my_mminer("dump_miner", labels);
    melon::var::Miner<int>* miner1 = my_mminer.get_stats(labels_value1);
    ASSERT_TRUE(miner1);
    *miner1 << 3 << 1 << 5;

    melon::var::Miner<int>* miner2 = my_mminer.get_stats(labels_value2);
    ASSERT_TRUE(miner2);
    *miner2 << 2 << 6 << 4;

    melon::var::Miner<int>* miner3 = my_mminer.get_stats(labels_value3);
    ASSERT_TRUE(miner3);
    *miner3 << 9 << 6 << 3;

    melon::var::MultiDimension<melon::var::LatencyRecorder> my_mlatencyrecorder("dump_latencyrecorder", labels);
    melon::var::LatencyRecorder* my_latencyrecorder1 = my_mlatencyrecorder.get_stats(labels_value1);
    ASSERT_TRUE(my_latencyrecorder1);
    *my_latencyrecorder1 << 1 << 3 << 5;
    *my_latencyrecorder1 << 2 << 4 << 6;
    *my_latencyrecorder1 << 3 << 6 << 9;
    sleep(2);
    
    google::SetCommandLineOption("var_dump_interval", old_var_dump_interval.c_str());
    google::SetCommandLineOption("mvar_dump", old_mvar_dump.c_str());
    google::SetCommandLineOption("mvar_dump_prefix", old_mvar_dump_prefix.c_str());
    google::SetCommandLineOption("mvar_dump_format", old_mvar_dump_format.c_str());
}

TEST_F(MVariableTest, test_describe_exposed) {
    std::list<std::string> labels_value1 {"bj", "get", "200"};
    std::string var_name("request_count_describe");
    melon::var::MultiDimension<melon::var::Adder<int> > my_madder1(var_name, labels);

    std::string describe_str = melon::var::MVariable::describe_exposed(var_name);

    std::ostringstream describe_oss;
    ASSERT_EQ(0, melon::var::MVariable::describe_exposed(var_name, describe_oss));
    ASSERT_STREQ(describe_str.c_str(), describe_oss.str().c_str());
}
