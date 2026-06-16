#pragma once

#include <cstdlib>
#include <format>
#include <iostream>
#include <print>
#include <source_location>
#include <string_view>
#include <utility>

namespace tasksat::log {

inline constexpr std::string_view RESET = "\033[0m";               // Reset
inline constexpr std::string_view BLACK = "\033[30m";              // Black
inline constexpr std::string_view RED = "\033[31m";                // Red
inline constexpr std::string_view GREEN = "\033[32m";              // Green
inline constexpr std::string_view BLUE = "\033[34m";               // Blue
inline constexpr std::string_view CYAN = "\033[36m";               // Cyan
inline constexpr std::string_view MAGENTA = "\033[35m";            // Magenta
inline constexpr std::string_view YELLOW = "\033[33m";             // Yellow
inline constexpr std::string_view WHITE = "\033[37m";              // White
inline constexpr std::string_view BOLDBLACK = "\033[1m\033[30m";   // Bold Black
inline constexpr std::string_view BOLDRED = "\033[1m\033[31m";     // Bold Red
inline constexpr std::string_view BOLDGREEN = "\033[1m\033[32m";   // Bold Green
inline constexpr std::string_view BOLDBLUE = "\033[1m\033[34m";    // Bold Blue
inline constexpr std::string_view BOLDCYAN = "\033[1m\033[36m";    // Bold Cyan
inline constexpr std::string_view BOLDMAGENTA = "\033[1m\033[35m"; // Bold Magenta
inline constexpr std::string_view BOLDYELLOW = "\033[1m\033[33m";  // Bold Yellow
inline constexpr std::string_view BOLDWHITE = "\033[1m\033[37m";   // Bold White

template <class... Args>
void info(std::format_string<Args...> fmt, Args&&... args) {
    std::print("{}[INFO]{} ", BOLDCYAN, RESET);
    std::println(fmt, std::forward<Args>(args)...);
}

template <class... Args>
void debug(std::format_string<Args...> fmt, Args&&... args) {
    std::print(std::cerr, "{}[DEBUG]{} ", BOLDGREEN, RESET);
    std::println(std::cerr, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void warning(std::format_string<Args...> fmt, Args&&... args) {
    std::print(std::cerr, "{}[WARNING]{} ", BOLDYELLOW, RESET);
    std::println(std::cerr, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void error(std::format_string<Args...> fmt, Args&&... args) {
    std::print(std::cerr, "{}[ERROR]{} ", BOLDRED, RESET);
    std::println(std::cerr, fmt, std::forward<Args>(args)...);
}

template <class... Args>
[[noreturn]] void assertion(const std::source_location& loc, std::format_string<Args...> fmt, Args&&... args) {
    std::println(std::cerr, "{}[FAILED]{} {} ({}: line {})", BOLDMAGENTA, RESET, loc.function_name(), loc.file_name(), loc.line());
    std::println(std::cerr, fmt, std::forward<Args>(args)...);
    std::abort();
}

} // namespace tasksat::log
