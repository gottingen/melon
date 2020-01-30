#include <test/testing/log_includes.h>

static const char *tested_logger_name = "null_logger";
static const char *tested_logger_name2 = "null_logger2";

TEST(registry, drop) {
    spdlog::drop_all();
    spdlog::create<spdlog::sinks::null_sink_mt>(tested_logger_name);
    EXPECT_TRUE(spdlog::get(tested_logger_name) != nullptr);
    // Throw if registring existing name
    EXPECT_THROW(spdlog::create<spdlog::sinks::null_sink_mt>(tested_logger_name), spdlog::spdlog_ex);
}

TEST(registry,
     explicitRegistry) {
    spdlog::drop_all();
    auto logger = std::make_shared<spdlog::logger>(tested_logger_name, std::make_shared<spdlog::sinks::null_sink_st>());
    spdlog::register_logger(logger);
    EXPECT_TRUE(spdlog::get(tested_logger_name) != nullptr);
    // Throw if registring existing name
    EXPECT_THROW(spdlog::create<spdlog::sinks::null_sink_mt>(tested_logger_name), spdlog::spdlog_ex);
}

TEST(registry, apply) {
    spdlog::drop_all();
    auto logger = std::make_shared<spdlog::logger>(tested_logger_name, std::make_shared<spdlog::sinks::null_sink_st>());
    spdlog::register_logger(logger);
    auto logger2 =
        std::make_shared<spdlog::logger>(tested_logger_name2, std::make_shared<spdlog::sinks::null_sink_st>());
    spdlog::register_logger(logger2);

    int counter = 0;
    spdlog::apply_all([&counter] (std::shared_ptr<spdlog::logger> l) { counter++; });
    EXPECT_TRUE(counter == 2);

    counter = 0;
    spdlog::drop(tested_logger_name2);
    spdlog::apply_all([&counter] (std::shared_ptr<spdlog::logger> l) {
        EXPECT_TRUE(l->name() == tested_logger_name);
        counter++;
    });
    EXPECT_TRUE(counter == 1);
}

TEST(registry,
     dorp1) {
    spdlog::drop_all();
    spdlog::create<spdlog::sinks::null_sink_mt>(tested_logger_name);
    spdlog::drop(tested_logger_name);
    EXPECT_FALSE(spdlog::get(tested_logger_name));
}

TEST(registry,
     dropall) {
    spdlog::drop_all();
    spdlog::create<spdlog::sinks::null_sink_mt>(tested_logger_name);
    spdlog::create<spdlog::sinks::null_sink_mt>(tested_logger_name2);
    spdlog::drop_all();
    EXPECT_FALSE(spdlog::get(tested_logger_name));
    EXPECT_FALSE(spdlog::get(tested_logger_name));
}

TEST(registry, drop_existing) {
    spdlog::drop_all();
    spdlog::create<spdlog::sinks::null_sink_mt>(tested_logger_name);
    spdlog::drop("some_name");
    EXPECT_FALSE(spdlog::get("some_name"));
    EXPECT_TRUE(spdlog::get(tested_logger_name));
    spdlog::drop_all();
}
