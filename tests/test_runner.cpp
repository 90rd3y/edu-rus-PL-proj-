#include "test_runner.hpp"
#include <cstdlib>
#include <exception>
#include <format>
#include <iostream>
#include <string_view>

void TestRunner::run(std::string_view name, void (*test_func)()) {
    try {
        test_func();
        std::cout << std::format("[PASS] {}\n", name);
        passed++;
    } catch (const std::exception& e) {
        std::cout << std::format("[FAIL] {}: {}\n", name, e.what());
        failed++;
    }
}

void TestRunner::summary() const {
    std::cout << std::format(
        "\n[SUMMARY] {}/{} tests passed.\n", passed, passed + failed);
    if (failed > 0) std::exit(1);
}
