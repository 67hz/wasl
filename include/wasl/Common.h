#ifndef WASL_COMMON_H
#define WASL_COMMON_H

#include <cassert>
#include <type_traits>

namespace wasl {

struct posix {};
struct windows {};
struct non_posix {};
struct darwin {};

#ifdef _WIN32
#define SYS_API_WIN32 1
using platform_type = windows;

#elif defined(__linux__) || defined(__APPLE__)
#define SYS_API_LINUX 1
using platform_type = posix;

#endif

using platform_type = posix;
template <typename T>
using EnableIfPlatform =
    std::enable_if_t<std::is_same<platform_type, T>::value>;

template <typename P1, typename P2, typename V = bool>
using EnableIfSamePlatform = std::enable_if_t<std::is_same<P1, P2>::value, V>;

} // namespace wasl

#endif // WASL_COMMON_H
