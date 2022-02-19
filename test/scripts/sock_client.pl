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
use IO::Socket::IP qw(:DEFAULT :crlf);
use constant MAX_MSG_LEN => 128;
$/ = CRLF;

my $sock_type = shift || "SOCK_STREAM";
my $host = shift || '127.0.0.1';
my $port = shift || '9877';
my $hangup = shift || "bye";
my $data = "";

my $sock_type_num = $sock_type eq "SOCK_STREAM" ? SOCK_STREAM : SOCK_DGRAM;

my $socket = IO::Socket::IP->new (
	Type => $sock_type_num,
	PeerHost => $host,
	Reuse => 1,
	PeerPort => $port) or die $@;

$socket->autoflush;

$socket->send("howdy\n") or die "send() failed: $!\n";

if ($sock_type_num eq SOCK_STREAM) {
	while (my $line = <$socket>) {
		chomp $line;
		print $socket $line;
	}
}
else {

	do {
		$socket->recv($data, MAX_MSG_LEN) or die "recv() failed: $!\n";
		chomp $data;
		# echo back to server
		#	$socket->send("$data");
		print $socket $data;
	} while $data ne $hangup;
}


close($socket);
