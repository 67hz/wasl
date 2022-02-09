#!/usr/bin/perl

###
#
# A TCP client
#
# usage: ./socket_client.pl {localhost} {port} {message}
#
##

use strict;
use warnings;
use IO::Socket;

my $socket = new IO::Socket::INET (
	PeerAddr => shift || 'localhost',
	PeerPort => shift || '9877',
	Proto => 'tcp',
);

die "Could not create socket: $!\n" unless $socket;

my $msg = shift || "placeholder message";
print $socket "$msg\n";

close($socket);
