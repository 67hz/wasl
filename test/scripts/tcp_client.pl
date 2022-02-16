#!/usr/bin/perl

###
#
# A basic TCP client to echo back the value of {message}
#
# usage: ./socket_client.pl {localhost} {port} {message}
#
##

use strict;
use warnings;
use IO::Socket qw(:DEFAULT :crlf);

my $socket = IO::Socket::INET->new (
	Proto => 'tcp',
	PeerAddr => shift || '127.0.0.1',
	PeerPort => shift || '9877'
) or die $@;


my $msg = shift || "placeholder message";
print $socket "$msg\n";

close($socket);
