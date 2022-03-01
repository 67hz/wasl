#include <iostream>
#include <wasl/SockStream.h>
#include <wasl/Socket.h>

#include "../test_helpers.h"

#include <array>
#include <fstream>
#include <string>
#include <algorithm>


#include <sys/select.h>
#include <sys/types.h>

using namespace wasl::ip;
using namespace std::string_literals;


int main(int argc, char *argv[])
{
	if (argc < 3)
		exit(EXIT_FAILURE);

	int max_fd;
	int nready;
	fd_set allset, readset;
	std::array<int, FD_SETSIZE> clients;
	clients.fill(-1);

	Address_info serverAddrInfo;
	serverAddrInfo.host = argv[1];
	serverAddrInfo.service = argv[2];
	serverAddrInfo.reuse_addr = true;

	auto server{make_socket_listener<AF_INET, SOCK_STREAM>(serverAddrInfo)};
	max_fd = sockno(*server);

	char buf[BUFSIZ];

	std::fstream journal("my_test.log"s, journal.binary | journal.trunc | journal.out);
	journal << "server running" << std::endl;

	FD_ZERO(&allset);
	FD_SET(sockno(*server), &allset);

	for (;;) {
		readset = allset;
		if ( (nready = select(max_fd + 1, &readset, nullptr, nullptr, nullptr)) == -1)
			return 1;

		if (FD_ISSET(sockno(*server), &readset)) { // new client connected to server
			auto client_fd = socket_accept(server.get());

			// fill first available entry
			auto it = std::find(clients.begin(), clients.end(), -1);
			if (it != clients.end()) {
				*it = client_fd;
				journal << "joined fd: " << client_fd << std::endl;
				std::cout << "joined fd: " << client_fd << '\n';
			} else {
				journal << "Connection Limit Exceeded\n";
				exit(EXIT_FAILURE);
			}

			FD_SET(client_fd, &allset);
			if (client_fd > max_fd)
				max_fd = client_fd;

			if (--nready <= 0)
				continue;		// no more readable sockets
		}

		for (auto client = clients.begin(); client != clients.end(); ++client) {
				if (*client < 0) continue;

				if (FD_ISSET(*client, &readset)) {
					auto cl_sockstream = sdopen(*client);
					*cl_sockstream >> buf;

					if (cl_sockstream->peek() == -1) {
						std::cout << *client << " closed connection\n";
						FD_CLR(*client, &allset);
						*client = -1;
					} else {
						if (strcmp(buf, SERVER_SHUTDOWN) == 0) {
							*cl_sockstream << "server_exit" << std::endl;
							journal << "server shut down by fd: " << *client << std::endl;
							close(sockno(*server));
							exit(EXIT_SUCCESS);
						} else {
							journal << "fd: " << *client << " : " << buf << std::endl;
							// echo back to client
							*cl_sockstream << buf << std::endl;
						}
					}

					if (--nready <= 0)
						break;
				}
		}
	}

	return 0;
}
