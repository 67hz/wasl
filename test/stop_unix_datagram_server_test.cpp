#include <wasl/Socket.h>
#include <wasl/SockStream.h>
#include "test_helpers.h"

#include <stdlib.h>
#include <gsl/span>

using namespace wasl::ip;

int main(int argc, char **argv)
{
	if (argc < 2) exit(EXIT_FAILURE);

	Address_info serverAddrInfo;
	serverAddrInfo.host = argv[1];

	auto tmpSock = wasl::makeSocketPath();

	std::cout <<  "tmpSock : " << tmpSock << '\n';

  auto client { make_socket<AF_UNIX, SOCK_DGRAM>({tmpSock.c_str()})};
  client->connect(serverAddrInfo);

	assert(client->error() == SockError::ERR_NONE);

	char buf[BUFSIZ];
	auto ss_cl { sdopen(sockno(*client)) };
	assert(is_valid_socket(sockno(*client)));
	*ss_cl << SERVER_SHUTDOWN << std::endl;

	*ss_cl >> buf;
//	recvfrom(sockno(*client), buf, BUFSIZ, 0, nullptr, 0);

	// TODO send_fd and check response from server
	assert(strcmp(buf, "server_exit") == 0);



	return 0;
}
