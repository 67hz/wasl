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

using namespace wasl::ip;
using namespace std::string_literals;



int main(int argc, char *argv[])
{
	auto listener_handle { make_socket<AF_UNIX, SOCK_DGRAM>({.host = SRV_PATH}) };
	auto listener_fd { sockno(*listener_handle) };

	auto muxer {make_muxer<SOCKET>()};
	if (!muxer->add(listener_fd)) {
		std::cerr << "iomux add main fd: " << listener_fd << ": "
			<< strerror(errno) << '\n';
		exit(EXIT_FAILURE);
	}

	std::atomic_int event_counter{0};

	labeled_handler<std::string> printer{
		"ev_print"s, [&event_counter, &muxer](SOCKET sfd, std::string data) {
			std::cout << "sfd triggered: " << sfd << "\nevent data:  " << data
				<< '\n';
			++event_counter;
			auto ss = sdopen(sfd);

			char buf[BUFSIZ];
			*ss >> buf;
			auto lastpeer { ss->last_peer() };
			struct sockaddr_un* peerAddr = reinterpret_cast<struct sockaddr_un*>(&lastpeer);

			if (strcmp(buf, "exit") == 0) {
				auto msg {"server_exit"};
				if (sendto(sfd, msg, sizeof(msg), 0, (SOCKADDR *)peerAddr, sizeof(sockaddr_un)) == -1) {
					std::cerr << "sending failed " << strerror(errno) << '\n';
				}
				close(sfd);
				return;
			}
			std::cout << "serverbuf: " << buf << std::endl;
			auto msg {"who somebody"};
			if (sendto(sfd, msg, sizeof(msg), 0, (SOCKADDR *)peerAddr, sizeof(sockaddr_un)) == -1) {
				std::cerr << "sending failed " << strerror(errno) << '\n';
			}
		}};

	// register event on muxers main listening sfd
	muxer->bind_event(listener_fd, printer);


	auto listen_n = [&event_counter, mux = muxer.get()]() {
		auto nfds = mux->listen();
//		assert(event_counter == 1);
	};

	for (;;) {
		listen_n();
	}


	return EXIT_SUCCESS;
}
