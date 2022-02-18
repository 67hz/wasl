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
use IO::Socket::IP qw(:DEFAULT :crlf);

my $socket = IO::Socket::IP->new (
	PeerHost => shift || '127.0.0.1',
	PeerPort => shift || '9877',
	Type => SOCK_STREAM,
) or die $@;


my $msg = shift || "placeholder message";
print $socket "$msg\n";

close($socket);
