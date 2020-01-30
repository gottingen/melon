#include <test/testing/log_includes.h>

static const char *tested_logger_name = "null_logger";
static const char *tested_logger_name2 = "null_logger2";

TEST(registry, drop) {
    abel::drop_all();
    abel::create<abel::sinks::null_sink_mt>(tested_logger_name);
    EXPECT_TRUE(abel::get(tested_logger_name) != nullptr);
    // Throw if registring existing name
    EXPECT_THROW(abel::create<abel::sinks::null_sink_mt>(tested_logger_name), abel::spdlog_ex);
}

TEST(registry,
     explicitRegistry) {
    abel::drop_all();
    auto logger = std::make_shared<abel::logger>(tested_logger_name, std::make_shared<abel::sinks::null_sink_st>());
    abel::register_logger(logger);
    EXPECT_TRUE(abel::get(tested_logger_name) != nullptr);
    // Throw if registring existing name
    EXPECT_THROW(abel::create<abel::sinks::null_sink_mt>(tested_logger_name), abel::spdlog_ex);
}

TEST(registry, apply) {
    abel::drop_all();
    auto logger = std::make_shared<abel::logger>(tested_logger_name, std::make_shared<abel::sinks::null_sink_st>());
    abel::register_logger(logger);
    auto logger2 =
        std::make_shared<abel::logger>(tested_logger_name2, std::make_shared<abel::sinks::null_sink_st>());
    abel::register_logger(logger2);

    int counter = 0;
    abel::apply_all([&counter] (std::shared_ptr<abel::logger> l) { counter++; });
    EXPECT_TRUE(counter == 2);

    counter = 0;
    abel::drop(tested_logger_name2);
    abel::apply_all([&counter] (std::shared_ptr<abel::logger> l) {
        EXPECT_TRUE(l->name() == tested_logger_name);
        counter++;
    });
    EXPECT_TRUE(counter == 1);
}

TEST(registry,
     dorp1) {
    abel::drop_all();
    abel::create<abel::sinks::null_sink_mt>(tested_logger_name);
    abel::drop(tested_logger_name);
    EXPECT_FALSE(abel::get(tested_logger_name));
}

TEST(registry,
     dropall) {
    abel::drop_all();
    abel::create<abel::sinks::null_sink_mt>(tested_logger_name);
    abel::create<abel::sinks::null_sink_mt>(tested_logger_name2);
    abel::drop_all();
    EXPECT_FALSE(abel::get(tested_logger_name));
    EXPECT_FALSE(abel::get(tested_logger_name));
}

TEST(registry, drop_existing) {
    abel::drop_all();
    abel::create<abel::sinks::null_sink_mt>(tested_logger_name);
    abel::drop("some_name");
    EXPECT_FALSE(abel::get("some_name"));
    EXPECT_TRUE(abel::get(tested_logger_name));
    abel::drop_all();
}
