#include <wasl/Socket.h>
#include <wasl/SockStream.h>

#include "../test_helpers.h"

using namespace wasl::ip;

int main(int argc, char *argv[]) {

	if (argc < 3)
		exit(EXIT_FAILURE);

	auto server { make_socket<AF_INET, SOCK_STREAM>({
							.service = argv[2],
							.host = argv[1],
							.reuse_addr = true}) };
	socket_listen(*server);

	char buf[BUFSIZ];

	FILE *journal = fopen("my_test.log", "w");

	fprintf(journal, "server running\n");
	auto client_fd = socket_accept(server.get());

	// open a IO socketstream
	auto client_ss = sdopen(client_fd);

	while (strstr(buf, "exit") == NULL) {

		// read from clients
		*client_ss >> buf;
		fprintf(journal, "received from %d: %s\n", client_fd, buf);

		// echo back
		*client_ss << "from_server:" << buf << std::endl;
	}

	return 0;
}
