#include <wasl/Socket.h>
#include <wasl/SockStream.h>

#include "../test_helpers.h"

using namespace wasl::ip;

int main() {
	auto server { make_socket<AF_INET, SOCK_STREAM>({
							.service = SERVICE,
							.host = HOST,
							.reuse_addr = true}) };
	socket_listen(*server);

	char buf[BUFSIZ];

	std::cout << "server is running..." << '\n';
	auto client_fd = socket_accept(server.get());
	// open a IO socketstream
	auto client_ss = sdopen(client_fd);

	while (strstr(buf, "exit") == NULL) {

		// read from clients
		*client_ss >> buf;

		// echo back
		*client_ss << "from server: " << buf << std::endl;
	}

	return 0;
}
