#include <test/testing/log_includes.h>

static const char *tested_logger_name = "null_logger";
static const char *tested_logger_name2 = "null_logger2";

TEST(registry, drop) {
    abel::log::drop_all();
    abel::log::create<abel::log::sinks::null_sink_mt>(tested_logger_name);
    EXPECT_TRUE(abel::log::get(tested_logger_name) != nullptr);
    // Throw if registring existing name
    EXPECT_THROW(abel::log::create<abel::log::sinks::null_sink_mt>(tested_logger_name), abel::log::log_ex);
}

TEST(registry,
     explicitRegistry) {
    abel::log::drop_all();
    auto logger = std::make_shared<abel::log::logger>(tested_logger_name, std::make_shared<abel::log::sinks::null_sink_st>());
    abel::log::register_logger(logger);
    EXPECT_TRUE(abel::log::get(tested_logger_name) != nullptr);
    // Throw if registring existing name
    EXPECT_THROW(abel::log::create<abel::log::sinks::null_sink_mt>(tested_logger_name), abel::log::log_ex);
}

TEST(registry, apply) {
    abel::log::drop_all();
    auto logger = std::make_shared<abel::log::logger>(tested_logger_name, std::make_shared<abel::log::sinks::null_sink_st>());
    abel::log::register_logger(logger);
    auto logger2 =
        std::make_shared<abel::log::logger>(tested_logger_name2, std::make_shared<abel::log::sinks::null_sink_st>());
    abel::log::register_logger(logger2);

    int counter = 0;
    abel::log::apply_all([&counter] (std::shared_ptr<abel::log::logger> l) { counter++; });
    EXPECT_TRUE(counter == 2);

    counter = 0;
    abel::log::drop(tested_logger_name2);
    abel::log::apply_all([&counter] (std::shared_ptr<abel::log::logger> l) {
        EXPECT_TRUE(l->name() == tested_logger_name);
        counter++;
    });
    EXPECT_TRUE(counter == 1);
}

TEST(registry,
     dorp1) {
    abel::log::drop_all();
    abel::log::create<abel::log::sinks::null_sink_mt>(tested_logger_name);
    abel::log::drop(tested_logger_name);
    EXPECT_FALSE(abel::log::get(tested_logger_name));
}

TEST(registry,
     dropall) {
    abel::log::drop_all();
    abel::log::create<abel::log::sinks::null_sink_mt>(tested_logger_name);
    abel::log::create<abel::log::sinks::null_sink_mt>(tested_logger_name2);
    abel::log::drop_all();
    EXPECT_FALSE(abel::log::get(tested_logger_name));
    EXPECT_FALSE(abel::log::get(tested_logger_name));
}

TEST(registry, drop_existing) {
    abel::log::drop_all();
    abel::log::create<abel::log::sinks::null_sink_mt>(tested_logger_name);
    abel::log::drop("some_name");
    EXPECT_FALSE(abel::log::get("some_name"));
    EXPECT_TRUE(abel::log::get(tested_logger_name));
    abel::log::drop_all();
}
