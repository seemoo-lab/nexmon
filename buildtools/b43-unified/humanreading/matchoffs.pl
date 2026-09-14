#!/usr/bin/perl

while(<STDIN>) {
  my @matches = ($_ =~ m/(0x[0-9a-fA-F]+,off\d+)/g);
  if (scalar @matches > 0) {
    foreach my $expr (@matches) {
      print "$expr\n";
    }
  }
}
