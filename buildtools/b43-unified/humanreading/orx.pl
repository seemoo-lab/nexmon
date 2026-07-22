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

	if($istr =~ /\A\s*orx\s*(.*)/) {
		my $args = "$1 ";
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
		$mask = ($mask << $S) | ($mask >> (16 - $S));
		my $xxx = $argsV[2];
		my $yyy = $argsV[3];
		my $zzz = $argsV[4];

		# questa stringa e' tmp
		my $tmpxxxl = "";
		my $tmpxxxr = "";

		# genera la stringa con il primo operando
		my $xstring;

		# se xxx e yyy sono numeri lascia cosi'
		if(isnumber($xxx) and isnumber($yyy)) {
			print "$_";
		}
		else {
			# se questo e' un numero allora calcola il risultato subito
			if(isnumber($xxx)) {
				my $num = getnumber($xxx);
				$xstring = sprintf("0x%04X", (($num << $S) | ($num >> (16-$S))) & $mask);
			} else {
				# se uno dei due shift esce dalla maschera allora sopprimilo
				my $tmpop = (0xFFFF << $S) & 0xFFFF;
				$tmpop = $tmpop & $mask;
				if($tmpop != 0) {
					$tmpxxxl = "($xxx << $S)";
				}
				$tmpop = (0xFFFF >> (16 - $S)) & 0xFFFF;
				$tmpop = $tmpop & $mask;
				if($tmpop != 0) {
 					my $n = 16 - $S;
					$tmpxxxr = "($xxx >> $n)";
				}

				# ora genera la stringa
				if(length($tmpxxxl) > 0 & length($tmpxxxr) > 0) {
					$xstring = "($tmpxxxl | $tmpxxxr)";
				} else {
					$xstring = "$tmpxxxl$tmpxxxr";
				}
				$xstring = "$xstring & ".sprintf("0x%04X", $mask);
			}
			print "\t// OLD CODE $_";
			print "\torxh\t$xstring, $yyy & ~".sprintf("0x%04X", $mask).", $zzz\n";
		}
	} else { print "$_"; }
}
