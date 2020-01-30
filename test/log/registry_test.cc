#include <test/testing/log_includes.h>

static const char *tested_logger_name = "null_logger";
static const char *tested_logger_name2 = "null_logger2";

TEST(registry, drop) {
    abel_log::drop_all();
    abel_log::create<abel_log::sinks::null_sink_mt>(tested_logger_name);
    EXPECT_TRUE(abel_log::get(tested_logger_name) != nullptr);
    // Throw if registring existing name
    EXPECT_THROW(abel_log::create<abel_log::sinks::null_sink_mt>(tested_logger_name), abel_log::spdlog_ex);
}

TEST(registry,
     explicitRegistry) {
    abel_log::drop_all();
    auto logger = std::make_shared<abel_log::logger>(tested_logger_name, std::make_shared<abel_log::sinks::null_sink_st>());
    abel_log::register_logger(logger);
    EXPECT_TRUE(abel_log::get(tested_logger_name) != nullptr);
    // Throw if registring existing name
    EXPECT_THROW(abel_log::create<abel_log::sinks::null_sink_mt>(tested_logger_name), abel_log::spdlog_ex);
}

TEST(registry, apply) {
    abel_log::drop_all();
    auto logger = std::make_shared<abel_log::logger>(tested_logger_name, std::make_shared<abel_log::sinks::null_sink_st>());
    abel_log::register_logger(logger);
    auto logger2 =
        std::make_shared<abel_log::logger>(tested_logger_name2, std::make_shared<abel_log::sinks::null_sink_st>());
    abel_log::register_logger(logger2);

    int counter = 0;
    abel_log::apply_all([&counter] (std::shared_ptr<abel_log::logger> l) { counter++; });
    EXPECT_TRUE(counter == 2);

    counter = 0;
    abel_log::drop(tested_logger_name2);
    abel_log::apply_all([&counter] (std::shared_ptr<abel_log::logger> l) {
        EXPECT_TRUE(l->name() == tested_logger_name);
        counter++;
    });
    EXPECT_TRUE(counter == 1);
}

TEST(registry,
     dorp1) {
    abel_log::drop_all();
    abel_log::create<abel_log::sinks::null_sink_mt>(tested_logger_name);
    abel_log::drop(tested_logger_name);
    EXPECT_FALSE(abel_log::get(tested_logger_name));
}

TEST(registry,
     dropall) {
    abel_log::drop_all();
    abel_log::create<abel_log::sinks::null_sink_mt>(tested_logger_name);
    abel_log::create<abel_log::sinks::null_sink_mt>(tested_logger_name2);
    abel_log::drop_all();
    EXPECT_FALSE(abel_log::get(tested_logger_name));
    EXPECT_FALSE(abel_log::get(tested_logger_name));
}

TEST(registry, drop_existing) {
    abel_log::drop_all();
    abel_log::create<abel_log::sinks::null_sink_mt>(tested_logger_name);
    abel_log::drop("some_name");
    EXPECT_FALSE(abel_log::get("some_name"));
    EXPECT_TRUE(abel_log::get(tested_logger_name));
    abel_log::drop_all();
}
