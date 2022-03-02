#include <wasl/IOMultiplexer.h>
#include <wasl/SockStream.h>
#include <wasl/Socket.h>

#include <atomic>
#include <functional>
#include <iterator>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include <gsl/string_span>

#include "test_helpers.h"
#include <gtest/gtest.h>

using namespace std::string_literals;
using namespace wasl::ip;

template <typename Muxer>
auto create_child_runner(Muxer *mux, SOCKET mux_fd, path_type spath, std::string msg) {
	return [mux, mux_fd, spath, msg]() {
		auto s_handle {wasl_socket<AF_UNIX, SOCK_DGRAM>::
			create()->connect(mux_fd)->build()
		};
		auto ss = sdopen(sockno(*s_handle));
		if (!s_handle)
			FAIL() << strerror(GET_SOCKERRNO()) << '\n';

		*ss << msg << std::endl;
		close(sockno(*s_handle));
	};
}

TEST(IOMuxEpollingDatagram, CanDispatchEventDelegates) {
	auto listener_handle { make_socket<AF_UNIX, SOCK_DGRAM>({.host = SRV_PATH}) };
	auto listener_fd { sockno(*listener_handle) };

	auto muxer {make_muxer<SOCKET>()};
	if (!muxer->add(listener_fd))
		FAIL() << "iomux add main fd: " << listener_fd << ": "
			<< strerror(errno) << '\n';

	std::atomic_int event_counter{0};

	labeled_handler<std::string> printer{
		"ev_print"s, [&event_counter](SOCKET sfd, std::string data) {
			std::cout << "sfd triggered: " << sfd << "\nevent data:  " << data
				<< '\n';
			++event_counter;
		}};

	// register event on muxers main listening sfd
	muxer->bind_event(listener_fd, printer);

	auto c1 = create_child_runner(muxer.get(), listener_fd, CL_PATH, "first");

	auto listen_n = [&event_counter, mux = muxer.get()](int num_events) {
		auto nfds = mux->listen();
		ASSERT_EQ(event_counter, 1);
	};

	std::thread writer(c1);
	wasl::thread_guard tw(writer);

	std::thread listener(listen_n, 3);
	wasl::thread_guard tl(listener);
}
