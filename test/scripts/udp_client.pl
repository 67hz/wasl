#!/usr/bin/perl

###
#
# A lightweight UDP client
#
# usage: ./udp_client.pl {path} {message}
#
##

use strict;
use warnings;
use IO::Socket;

$handle = IO::Socket::INET->new(Proto => "udp"))
	or die "socket: $!";

die "Could not create socket: $!\n" unless $socket;

my $msg = shift || "placeholder message";
print $socket "$msg\n";

close($socket);
