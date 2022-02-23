#include <wasl/SockStream.h>
#include <wasl/Socket.h>

#include "../test_helpers.h"

#include <fstream>
#include <string>

using namespace wasl::ip;
using namespace std::string_literals;

int main(int argc, char *argv[])
{

    if (argc < 3)
        exit(EXIT_FAILURE);

    auto server{make_socket<AF_INET, SOCK_STREAM>({.service = argv[2], .host = argv[1], .reuse_addr = true})};
    socket_listen(*server);

    char buf[BUFSIZ];

		std::fstream journal("my_test.log"s, journal.binary | journal.trunc | journal.out);

		journal << "server running\n";
//    fprintf(journal, "server running\n");
    auto client_fd = socket_accept(server.get());

    // open a IO socketstream
    auto client_ss = sdopen(client_fd);

    while (strcmp(buf, "exit") != 0)
    {
        // read from clients
        *client_ss >> buf;
				journal << "received from " << client_fd << ": " << buf << '\n';

        // echo back
        *client_ss << "from_server:" << buf << std::endl;
    }

    return 0;
}
