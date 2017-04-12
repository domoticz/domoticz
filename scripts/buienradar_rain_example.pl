#!/usr/bin/perl -w

=head1 buienradar_rain.pl

Installation:

Perl, you need to install the LWP module:
sudo apt-get install libjson-perl

In Domoticz create a Dummy Percentage sensor.
Next go to the Devices overview, and write down the 'idx' value of the sensor.

Copy this file to another location and edit the setting below to match your situation.

cp /home/pi/domoticz/scripts/buienradar_rain_example.pl /home/pi/domoticz/buienradar_rain.pl
nano /home/pi/domoticz/buienradar_rain.pl

Next add a Crontab rule:

crontab -e

Add the following line at the end:

*/5 * * * * perl /home/pi/domoticz/buienradar_rain.pl

In 5 minutes, the sensor should work

=cut

use strict;
use LWP::UserAgent;
use HTTP::Cookies;

my $lat=50.22041;
my $long=5.89744;
my $domoticz_ip="127.0.0.1";
my $domoticz_port="8080";
my $domoticz_sensor_idx="2442";
my $duration=15;

my @user_agents = (
		'Mozilla/4.0 (compatible; MSIE 4.01; Windows 98)'
	);

my $ua = LWP::UserAgent->new(
		#Set agent name, we are not a script! :)
		agent		=> $user_agents[rand @user_agents],
		cookie_jar	=> HTTP::Cookies->new(),
	);

my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime();

my $startTime=($hour*60)+$min;
my $endTime=$startTime+$duration;

my $url = "http://gps.buienradar.nl/getrr.php?lat=$lat&lon=$long";
my $response = $ua->get($url);

unless ($response->is_success) {
	# hmm, let's retry
	$response = $ua->get($url);

	unless ($response->is_success) {
		# still no luck; sleep and retry
		sleep 1;
		$response = $ua->get($url);

		unless ($response->is_success) {
			print "Could not connect to buienradar.nl.\n";
			exit 0;
		}
	}
}

my $data = $response->content;

unless ($data =~ /\A((\d{3})?\|\d{2}:\d{2}\r?\n)+\z/) {
	print "Could not parse the data returned by buienradar.nl.\n";
	exit 0;
}

my $total_rain_predictions=0;
my $total_rain_values=0;

while ($data =~ s/\A(\d{3})?\|(\d{2}:\d{2})\r?\n//) {
	my ($value, $mtime) = ($1, $2);

	if (defined $value) {
	
		my @hour_min = split(':', $mtime);
		my $mhour = $hour_min[0];
		my $mmin = $hour_min[1];
		my $calc_time=($mhour*60)+$mmin;
		if (($calc_time>=$startTime)&&($calc_time<=$endTime)) {
			$value =~ s/\A0+(\d)/$1/;
			$total_rain_predictions+=$value;
			$total_rain_values+=1;
		}
	}
}

my $result = "0.0";

if ($total_rain_values!=0) {
	my $rain_0_100=($total_rain_predictions/$total_rain_values)*0.392156862745098;
	$result = sprintf("%.2f", $rain_0_100);
}

$url = "http://$domoticz_ip:$domoticz_port/json.htm?type=command&param=udevice&idx=$domoticz_sensor_idx&nvalue=0&svalue=$result";
$response = $ua->get($url);

unless ($response->is_success) {
    print "Error sending data to domoticz!\n";
    exit 0;
}
$data = $response->content;

if (index($data, "\"OK\"") == -1) {
    print "Error sending data to domoticz!\n";
    exit 0;
}

print "OK, precip=$result\n";
