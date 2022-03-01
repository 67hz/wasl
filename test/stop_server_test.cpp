#include <wasl/Socket.h>
#include <wasl/SockStream.h>

#include <gsl/span>
using namespace wasl::ip;

int main(int argc, char **argv)
{
	if (argc < 3)
		exit(EXIT_FAILURE);

	Address_info serverAddrInfo;
	serverAddrInfo.host = argv[1];
	serverAddrInfo.service = argv[2];

	auto client {
		wasl_socket<AF_INET, SOCK_STREAM>::create()
			->connect (serverAddrInfo)
			->build()
	};

	char buf[BUFSIZ];
	struct sockaddr_in addr;
	auto ss_cl { sdopen(sockno(*client)) };
	assert(is_valid_socket(sockno(*client)));

	*ss_cl << "exit" << std::endl;

	//*ss_cl >> buf;

	//std::cout << "exit buf:" << buf << std::endl;
	return 0;
}
