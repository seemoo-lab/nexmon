#!/usr/bin/perl

sub isnumber
{
	if($_[0] =~ /\A0x[0-9a-fA-F]+\Z/ or $_[0] =~ /\A[0-9]+\Z/) {
		return 1;
	} else {
		return 0;
	}
}

sub getnumber
{
	my $num = $_[0];
	if($num =~ /\A0x/) {
		$num = hex($num);
	} else {
		$num = int($num);
	}
	return $num;
}

while(<STDIN>) {

	my $istr = $_;
	chomp $istr;

	if($_ =~ /\:(.*)/) {
		$istr = $2;
	} 

	if($istr =~ /\A\s*j([n]{0,1})zx\s*(.*)/) {
		my $args = "$2 ";
		my $non = $1;
		my @argsV;
		while(length($args) > 0) {
	
			if($args =~ /\A\s*\[(.*?)\][\s,]+(.*)/) {
				$args = $2;
				if($1 =~ /(.*?),(off\d+)/) {
					push(@argsV, "[$1,$2]");
				} else {
					push(@argsV, "[$1]");
				}
			} elsif($args =~ /\A\s*(.*?)[\s,]+(.*)/) {
				push(@argsV, $1);
				$args = $2;
			} else {
				last;
			}
		}

		my $M = $argsV[0];
		my $S = $argsV[1];
		my $mask = (1 << ($M + 1)) - 1;
		$mask = ($mask << $S);
		my $xxx = $argsV[2];
		my $yyy = $argsV[3];
		my $zzz = $argsV[4];

		# questa stringa e' tmp
		my $tmpxxxl = "";
		my $tmpxxxr = "";

		# genera la stringa con il primo operando
		my $string;

		# se xxx e yyy sono numeri lascia cosi'
		if(isnumber($xxx) and isnumber($yyy)) {
			print "$_";
		}
		else {
			# se questo e' un numero allora calcola il risultato subito
			if(isnumber($xxx) and getnumber($xxx) == 0) {
				#$string = sprintf("($yyy << 16) & $mask");
				$string = sprintf("($yyy << 16) & 0x%04X", $mask);
			} elsif(isnumber($yyy) and getnumber($yyy) == 0) {
				#$string = sprintf("$xxx & $mask");
				$string = sprintf("$xxx & 0x%04X", $mask);
			} else {
				#$string = sprintf("($yyy << 16 | $xxx) & $mask");
				$string = sprintf("($yyy << 16 | $xxx) & 0x%04X", $mask);
			}

			print "\t// OLD CODE $_";
			print "\tj".$non."zxh\t$string, $zzz\n";
		}
	} else { print "$_"; }
}
