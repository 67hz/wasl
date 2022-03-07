#include <iostream>
#include <wasl/SockStream.h>
#include <wasl/Socket.h>
#include <wasl/IOMultiplexer.h>

#include "../test_helpers.h"

#include <array>
#include <fstream>
#include <string>
#include <algorithm>
#include <atomic>


#include <sys/select.h>
#include <sys/types.h>
#include <setjmp.h>

using namespace wasl::ip;
using namespace std::string_literals;

static jmp_buf jmpbuffer;

static auto get_printer(const io_mux_base<int, epoll_muxer<int>>& muxer,
		wasl_socket<AF_UNIX, SOCK_DGRAM> &listener_handle, std::atomic_int &counter) {
	return labeled_handler<std::string> {
		"ev_print"s, [&muxer, &listener_handle, &counter](SOCKET sfd, std::string data) {
			std::cout << "sfd triggered: " << sfd << "\nevent data:  " << data
				<< '\n';
			++counter;
			auto ss = sdopen(sfd);

			char buf[BUFSIZ];
			*ss >> buf;
			auto lastpeer { ss->last_peer() };
			struct sockaddr_un* peerAddr = reinterpret_cast<struct sockaddr_un*>(&lastpeer);

			if (strcmp(buf, "exit") == 0) {
				auto msg {"server_exit\n"};
				int ret = sendto(sfd, msg, strlen(msg), 0, (SOCKADDR *)peerAddr, sizeof(sockaddr_un));
				listener_handle.close();
				if (ret < 0)
					exit(EXIT_FAILURE);
				else
					exit(EXIT_SUCCESS);
			}
			std::cout << "serverbuf: " << buf << std::endl;
			auto msg {"whosomebody"};
			if (sendto(sfd, msg, sizeof(msg), 0, (SOCKADDR *)peerAddr, sizeof(sockaddr_un)) == -1) {
				std::cerr << "sending failed " << strerror(errno) << '\n';
			}
		}};
}

int main(int argc, char *argv[])
{
	if (argc < 2)
		exit(EXIT_FAILURE);

	auto listener_handle { make_socket<AF_UNIX, SOCK_DGRAM>({.host = argv[1]}) };
	auto listener_fd { sockno(*listener_handle) };

	auto muxer {make_muxer<SOCKET>()};
	if (!muxer->add(listener_fd)) {
		std::cerr << "iomux add main fd: " << listener_fd << ": "
			<< strerror(errno) << '\n';
		exit(EXIT_FAILURE);
	}

	std::atomic_int event_counter{0};

	// register event on muxers main listening sfd
	muxer->bind_event(listener_fd, get_printer(*muxer, *listener_handle, event_counter));

	for (;;) {
		muxer->listen();
	}


}
