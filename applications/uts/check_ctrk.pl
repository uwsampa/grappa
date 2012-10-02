#!/usr/bin/perl
# On Cray:
#!/opt/open/open/bin/perl


while (<>) {
	next unless (/^CTRK/);

	# Capture the work ID into $1
	/(0x[0-9A-Fa-f]+$)/;
	$id = $1;

	if (/put chunk/) {
		$hash{$id}++;
		$nreleased++;
	}
	elsif (/got chunk/) {
		$hash{$id}--;
		$nacquired++;
	}
#	elsif (!/TERMINATING/) {
#		print "Warning: malformed entry.  $_";
#	}
}

print "Total Put = " . $nreleased . ", Total Got = " . $nacquired . "\n";

$errors = 0;

while(($key, $value) = each %hash) {
	($value > 0) and print "Never got: $key ($value)\n" and $errors++;
	($value < 0) and print "Never put: $key ($value)\n" and $errors++;
}

print "$errors errors\n";
