#!/usr/bin/perl

###
#
# A lightweight UDP echo client
#
# usage: ./udp_client.pl {host} {port} {termination_string}
#
# termination_string - message to send to signal client to close socket
#
##

use strict;
use IO::Socket qw(:DEFAULT :crlf);
use constant MAX_MSG_LEN => 128;
$/ = CRLF;

my $host = shift || '127.0.0.1';
my $port = shift || '9877';
my $hangup = shift || "bye";
my $data = "";

my $socket = IO::Socket::INET->new (
	Proto => 'udp',
	PeerHost => $host,
	PeerPort => $port) or die $@;

$socket->send("howdy\n") or die "send() failed: $!\n";

do {
	$socket->recv($data, MAX_MSG_LEN) or die "recv() failed: $!\n";
	chop($data);
	# echo back to server
	$socket->send("$data");
} while $data ne $hangup;


close($socket);
