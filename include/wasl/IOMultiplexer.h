#ifndef WASL_IOMULTIPLEXER_H
#define WASL_IOMULTIPLEXER_H

#include <wasl/Common.h>
#include <wasl/Types.h>

#include <functional>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

#ifdef SYS_API_LINUX
#include <sys/epoll.h>
#include <sys/unistd.h>
#endif

#ifdef SYS_API_WIN32
using epoll_event = std::nullptr_t;
#endif

#include <iostream>

namespace wasl {
namespace ip {

using socket_handler_fun = std::function<void(SOCKET, std::string)>;

template <typename Label, typename Callable = socket_handler_fun>
using labeled_handler = std::pair<Label, Callable>;

/// Hold a map of labeled callables for event delegation.
/// This is like a decomposed command pattern using a map of generic lambdas,
/// functors, or any other invokable as the commands.
template <typename T, typename L>
using event_map = std::map<T, labeled_handler<L>>;

template <typename T, typename Muxer> class io_mux_base : Muxer {
public:
  io_mux_base() { _listener_fd = this->init(); }

  int listen() {
    // pull fds with ready input
    auto ready_fds = this->wait(_listener_fd);
    if (!ready_fds.empty()) {
      for (auto fd : ready_fds) {
        auto it = _event_handlers.find(fd);

        if (it != _event_handlers.end())
          it->second.second(fd, "iomux event triggered: " + it->second.first);
      }
    }

    return ready_fds.size();
  }

  /// Bind an event handler to a socket descriptor.
  /// The event will be triggered upon reception of input on sfd.
  template <typename U> void bind_event(T fd, labeled_handler<U> f) {
    // todo add proper find, replace in another class.
    // move all insert, remove, logic out to allow other muxers to use as a
    // template param
    _event_handlers.insert(
        std::make_pair(fd, std::forward<labeled_handler<U>>(f)));
  }

  /// add a handle to the interest list
  bool add(T fd) {
    auto handle_added = this->link_node(_listener_fd, fd);

    if (!handle_added) { // successfully added
      return false;
    }
    _active_socket_list.insert(fd);
    return handle_added;
  }

private:
  T _listener_fd; // fd for listener/acceptor
  std::set<T> _active_socket_list;
  event_map<T, std::string> _event_handlers;
};

template <typename T, typename = void> struct epoll_muxer {
  using event_list = std::vector<T>;
	static T init() {
		std::cout << "default muxer\n";
		T other;
		return other;
	}

	static bool link_node(T poll_fd, T sfd) {
		return false;
	}

	static event_list wait(T poll_fd) {
	    event_list ev_list;
	    return ev_list;
	}
};

/// epoll() based event muxer
/// TODO check for >2.6 linux, and fix enableif switch
/// \note all static members for EBCO
template <typename T>
struct epoll_muxer<T, EnableIfPlatform<posix>> {
  using event_type = epoll_event;
  static constexpr int event_max = 10; // max events to fetch at a time
  using event_list = std::vector<T>;

  /// create a new epoll instance.
  ///
  /// \return file descriptor for primary listening (epoll) socket
  static T init() {
		std::cout << "epoll muxer\n";
	  return epoll_create(event_max);
  }

  static bool link_node(T poll_fd, T sfd) {
    struct epoll_event ev;
    ev.data.fd = sfd;
    ev.events = EPOLLIN;

    int result = epoll_ctl(poll_fd, EPOLL_CTL_ADD, sfd, &ev);

    return result == 0 ? true : false;
  }

  /// Listen for changes to any descriptors held in evlist.
  /// return number of ready file descriptors
  ///
  /// \return 0 (success), -1 (error), n (number of fds ready)
  static event_list wait(T poll_fd) {
    struct epoll_event events[event_max];
    event_list ev_list;
    auto nr_events = epoll_wait(poll_fd, events, event_max, -1);

    /// \todo on close events remove ev.data.fd from event delegates map and
    /// move associated handler to another socket.
    for (auto ev : events) {
      if (ev_list.size() >= nr_events) {
        return ev_list;
      }
      if (ev.data.fd > 0) {
        ev_list.push_back(ev.data.fd);
      }
    }

    return ev_list;
  }
};

template <typename T, class Muxer = epoll_muxer<T>>
auto make_muxer() {
  return std::make_unique<io_mux_base<T, Muxer>>();
}

} // end namespace ip
} // end namespace wasl

#endif /* WASL_IOMULTIPLEXER_H */
