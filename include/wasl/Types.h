#ifndef WASL_TYPES_H
#define WASL_TYPES_H

#include <memory>
#include <type_traits>

#ifdef SYS_API_LINUX
#include <netinet/in.h> // sockaddr_in
#include <sys/socket.h> // struct sockaddr
#endif

#ifdef __cplusplus
#define BEGIN_C_DECLS extern "C" {
#define END_C_DECLS }
#else /* __cplusplus */
#define BEGIN_C_DECLS
#define END_C_DECLS
#endif /* !__cplusplus */

#define WASL_NO_COPY(class_name)                                             \
  class_name(const class_name &) = delete;                                     \
  class_name &operator=(const class_name &) = delete;

// enable logical ops on an enum. CWCID - lldb-enumerations.h
#define WASL_MARK_AS_BITMASK_ENUM(Enum)                                      \
  constexpr Enum operator|(Enum a, Enum b) {                                   \
    return static_cast<Enum>(wasl::local::toUType(a) |                       \
                             wasl::local::toUType(b));                       \
  }                                                                            \
                                                                               \
  constexpr Enum operator&(Enum a, Enum b) {                                   \
    return static_cast<Enum>(wasl::local::toUType(a) &                       \
                             wasl::local::toUType(b));                       \
  }                                                                            \
                                                                               \
  constexpr Enum operator~(Enum a) {                                           \
    return ~static_cast<Enum>(wasl::local::toUType(a));                      \
  }                                                                            \
                                                                               \
  constexpr Enum &operator|=(Enum &a, Enum b) {                                \
    a = a | b;                                                                 \
    return a;                                                                  \
  }                                                                            \
                                                                               \
  constexpr Enum &operator&=(Enum &a, Enum b) {                                \
    a = a & b;                                                                 \
    return a;                                                                  \
  }

template <typename T>
struct is_socket
    : std::integral_constant<bool,
                             std::is_same<T, struct sockaddr_un>::value ||
                                 std::is_same<T, struct sockaddr_in>::value ||
                                 std::is_same<T, struct sockaddr_in6>::value> {
};

template <typename T, typename V = bool>
using EnableIfSocketType = std::enable_if_t<is_socket<T>::value, V>;

#ifndef _WIN32 /* Posix */

constexpr static int INVALID_SOCKET = -1;
using SOCKET = int;
using SOCKADDR = struct sockaddr;
#define GET_SOCKERRNO() (errno)
#define closesocket(x) close((x))
constexpr bool is_valid_socket(SOCKET s) { return s != INVALID_SOCKET; }

#else /* Win32 */

#define GET_SOCKERRNO() (WSAGetLastError())
inline constexpr bool is_valid_socket(SOCKET s) { return s >= 0; }

#endif

namespace wasl {

template <class T> class non_copyable {
protected:
  non_copyable() {}

public:
  non_copyable(const non_copyable &) = delete;
  non_copyable &operator=(const non_copyable &) = delete;
};

namespace local {
/// Get underlying type from enum - see Modern Meyers p72
template <typename Enum> constexpr auto toUType(Enum enumerator) noexcept {
  return static_cast<std::underlying_type_t<Enum>>(enumerator);
}
} // namespace local

namespace ip {

enum class SockFlags : uint32_t {
  DATAGRAM = 0x1,  // !DATAGRAM <= stream based (default)
  INTERHOST = 0x2, // interhost communication enabled
};
WASL_MARK_AS_BITMASK_ENUM(SockFlags);

enum class SockError : uint32_t {
  ERR_NONE = 0x0,
  ERR_SOCKET = 0x1,
  ERR_BIND = 0x2,
  ERR_CONNECT = 0x4,
  ERR_LISTEN = 0x8,
  ERR_PATH_INVAL = 0x10
};

WASL_MARK_AS_BITMASK_ENUM(SockError);
} // namespace ip

} // namespace wasl

#endif /* WASL_TYPES_H */
