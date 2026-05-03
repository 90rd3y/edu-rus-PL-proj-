#pragma once

#include <cstddef>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string_view>

struct TestRunner {
    size_t passed = 0;
    size_t failed = 0;

    void run(std::string_view name, void (*test_func)());
    void summary() const;
};

// Exception-based assertion for the test runner
#define ASSERT(cond, msg)                                                      \
    do {                                                                       \
        if (!(cond))                                                           \
            throw std::runtime_error(std::format(                              \
                "Assertion failed: {}:{} - {}", __FILE__, __LINE__, msg));     \
    } while (false)
