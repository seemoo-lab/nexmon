#!/usr/bin/perl

my %addressesdecimal;

while(<STDIN>) {
  my @matches = ($_ =~ m/\[(0x[0-9a-fA-F]+)\]/g);
  if (scalar @matches > 0) {
    foreach my $hex_addr (@matches) {
      my $dec_addr = hex($hex_addr);
      $addressesdecimal{$dec_addr} = 1;
    }
  }
  my @matches = ($_ =~ m/\[([0-9]+)\]/g);
  if (scalar @matches > 0) {
    foreach my $dec_addr (@matches) {
      $addressesdecimal{$dec_addr} = 1;
    }
  }
}

foreach my $dec_addr (sort {$a <=> $b} keys(%addressesdecimal)) {
  printf("0x%X\n", $dec_addr);
}
