#!/usr/bin/perl -w
#
# Copyright 2004 Jörg Mayer (see AUTHORS file)
#
# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
# Copyright 1998 Gerald Combs
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

# See below for usage
#
# If "version.conf" is present, it is parsed for configuration values.
# Possible values are:
#
#   enable       - Enable or disable versioning.  Zero (0) disables, nonzero
#		   enables.
#   svn_client   - Use svn client i.s.o. ugly internal SVN file hack
#   tortoise_svn - Use TortoiseSVN client instead of ugly internal SVN
#		   file hack
#   format       - A strftime() formatted string to use as a template for
#		   the version string. The sequence "%#" will substitute
#		   the number of commits for Git or the revision number
#		   for SVN.
#   pkg_enable   - Enable or disable local package versioning.
#   pkg_format   - Like "format", but used for the local package version.
#
# If run with the "-r" or "--set-release" argument the AC_INIT macro in
# configure.ac and the VERSION macro in CMakeLists.txt will have the
# pkg_format template appended to the version number. version.h will
# _not_ be generated if either argument is present.
#
# Default configuration:
#
# enable: 1
# git_client: 0
# svn_client: 0
# tortoise_svn: 0
# format: git %Y%m%d%H%M%S
# pkg_enable: 1
# pkg_format: -%#

# XXX - We're pretty dumb about the "%#" substitution, and about having
# spaces in the package format.

use strict;

use Time::Local;
use File::Basename;
use File::Spec;
use POSIX qw(strftime);
use Getopt::Long;
use Pod::Usage;
use IO::Handle;
use English;

my $version_file = 'version.h';
my $package_string = "";
my $vconf_file = 'version.conf';
my $vcs_name = "Git";
my $tortoise_file = "tortoise_template";
my $last_change = 0;
my $num_commits = 0;
my $commit_id = '';
my $repo_branch = "unknown";
my $git_executable = "git";
my $git_description = undef;
my $get_vcs = 0;
my $set_vcs = 0;
my $print_vcs = 0;
my $set_version = 0;
my $set_release = 0;
my %version_pref = (
	"version_major" => 2,
	"version_minor" => 2,
	"version_micro" => 3,
	"version_build" => 0,

	"enable"        => 1,
	"git_client"    => 0,	# set if .git found and .git/svn not found
	"svn_client"    => 0,	# set if .svn found
	"tortoise_svn"  => 0,
	"format"        => "git %Y%m%d%H%M%S",

	# Normal development builds
	"pkg_enable" => 1,
	"pkg_format" => "-%#",

	# Development releases
	#"pkg_enable" => 0,
	#"pkg_format" => "",
	);
my $srcdir = ".";
my $info_cmd = "";
my $verbose = 0;
my $devnull = File::Spec->devnull();
my $enable_vcsversion = 1;

# Ensure we run with correct locale
$ENV{LANG} = "C";
$ENV{LC_ALL} = "C";
$ENV{GIT_PAGER} = "";

sub print_diag {
	print STDERR @_ if $verbose;
}

# Attempt to get revision information from the repository.
sub read_repo_info {
	my $line;
	my $version_format = $version_pref{"format"};
	my $package_format = "";
	my $in_entries = 0;
	my $svn_name;
	my $repo_version;
	my $do_hack = 1;
	my $info_source = "Unknown";
	my $is_git_repo = 0;
	my $git_cdir;


	if ($version_pref{"pkg_enable"} > 0) {
		$package_format = $version_pref{"pkg_format"};
	}

	# For tarball releases, do not invoke git at all and instead rely on
	# versioning information that was provided at tarball creation time.
	if ($version_pref{"git_description"}) {
		$info_source = "version.conf file";
	} elsif (-e "$srcdir/.git" && ! -d "$srcdir/.git/svn") {
		$info_source = "Command line (git)";
		$version_pref{"git_client"} = 1;
		$is_git_repo = 1;
	} elsif (-d "$srcdir/.svn" or -d "$srcdir/../.svn") {
		$info_source = "Command line (svn info)";
		$info_cmd = "cd $srcdir; svn info";
		$version_pref{"svn_client"} = 1;
	} elsif (-d "$srcdir/.git/svn") {
		$info_source = "Command line (git-svn)";
		$info_cmd = "(cd $srcdir; $git_executable svn info)";
		$is_git_repo = 1;
	}

	# Make sure git is available.
	if ($is_git_repo && !`$git_executable --version`) {
		print STDERR "Git unavailable. Git revision will be missing from version string.\n";
		return;
	}

	# Check whether to include VCS version information in version.h
	if ($is_git_repo) {
		chomp($git_cdir = qx{git --git-dir="$srcdir/.git" rev-parse --git-common-dir 2> $devnull});
		if ($git_cdir && -f "$git_cdir/wireshark-disable-versioning") {
			print_diag "Header versioning disabled using git override.\n";
			$enable_vcsversion = 0;
		}
	}

	#Git can give us:
	#
	# A big ugly hash: git rev-parse HEAD
	# 1ddc83849075addb0cac69a6fe3782f4325337b9
	#
	# A small ugly hash: git rev-parse --short HEAD
	# 1ddc838
	#
	# The upstream branch path: git rev-parse --abbrev-ref --symbolic-full-name @{upstream}
	# origin/master
	#
	# A version description: git describe --tags --dirty
	# wireshark-1.8.12-15-g1ddc838
	#
	# Number of commits in this branch: git rev-list --count HEAD
	# 48879
	#
	# Number of commits since 1.8.0: git rev-list --count 5e212d72ce098a7fec4332cbe6c22fcda796a018..HEAD
	# 320
	#
	# Refs: git ls-remote code.wireshark.org:wireshark
	# ea19c7f952ce9fc53fe4c223f1d9d6797346258b (r48972, changed version to 1.11.0)

	if ($version_pref{"git_description"}) {
		$git_description = $version_pref{"git_description"};
		$do_hack = 0;
		# Assume format like v2.3.0rc0-1342-g7bdcf75
		$commit_id = ($git_description =~ /([0-9a-f]+)$/)[0];
	} elsif ($version_pref{"git_client"}) {
		eval {
			use warnings "all";
			no warnings "all";

			chomp($line = qx{$git_executable --git-dir="$srcdir"/.git log -1 --pretty=format:%at});
			if ($? == 0 && length($line) > 1) {
				$last_change = $line;
			}

			# Commits since last annotated tag.
			chomp($line = qx{$git_executable --git-dir="$srcdir"/.git describe --long --always --match "v*"});
			if ($? == 0 && length($line) > 1) {
				my @parts = split(/-/, $line);
				$git_description = $line;
				$num_commits = $parts[-2];
				$commit_id = $parts[-1];
			}

			# This will break in some cases. Hopefully not during
			# official package builds.
			chomp($line = qx{$git_executable --git-dir="$srcdir"/.git rev-parse --abbrev-ref --symbolic-full-name \@\{upstream\} 2> $devnull});
			if ($? == 0 && length($line) > 1) {
				$repo_branch = basename($line);
			}

			1;
		};

		if ($last_change && $num_commits && $repo_branch) {
			$do_hack = 0;
		}
	} elsif ($version_pref{"svn_client"}) {
		my $repo_root = undef;
		my $repo_url = undef;
		eval {
			use warnings "all";
			no warnings "all";
			$line = qx{$info_cmd};
			if (defined($line)) {
				if ($line =~ /Last Changed Date: (\d{4})-(\d\d)-(\d\d) (\d\d):(\d\d):(\d\d)/) {
					$last_change = timegm($6, $5, $4, $3, $2 - 1, $1);
				}
				if ($line =~ /Last Changed Rev: (\d+)/) {
					$num_commits = $1;
				}
				if ($line =~ /URL: (\S+)/) {
					$repo_url = $1;
				}
				if ($line =~ /Repository Root: (\S+)/) {
					$repo_root = $1;
				}
				$vcs_name = "SVN";
			}
			1;
		};

		if ($repo_url && $repo_root && index($repo_url, $repo_root) == 0) {
			$repo_branch = substr($repo_url, length($repo_root));
		}

		if ($last_change && $num_commits && $repo_url && $repo_root) {
			$do_hack = 0;
		}
	} elsif ($version_pref{"tortoise_svn"}) {
		# Dynamically generic template file needed by TortoiseSVN
		open(TORTOISE, ">$tortoise_file");
		print TORTOISE "#define VCSVERSION \"\$WCREV\$\"\r\n";
		print TORTOISE "#define VCSBRANCH \"\$WCURL\$\"\r\n";
		close(TORTOISE);

		$info_source = "Command line (SubWCRev)";
		$info_cmd = "SubWCRev $srcdir $tortoise_file $version_file";
		my $tortoise = system($info_cmd);
		if ($tortoise == 0) {
			$do_hack = 0;
		}
		$vcs_name = "SVN";

		#clean up the template file
		unlink($tortoise_file);
	}

	if ($num_commits == 0 and -e "$srcdir/.git") {

		# Try git...
		eval {
			use warnings "all";
			no warnings "all";
			# If someone had properly tagged 1.9.0 we could also use
			# "git describe --abbrev=1 --tags HEAD"

			$info_cmd = "(cd $srcdir; $git_executable log --format='%b' -n 1)";
			$line = qx{$info_cmd};
			if (defined($line)) {
				if ($line =~ /svn path=.*; revision=(\d+)/) {
					$num_commits = $1;
				}
			}
			$info_cmd = "(cd $srcdir; $git_executable log --format='%ad' -n 1 --date=iso)";
			$line = qx{$info_cmd};
			if (defined($line)) {
				if ($line =~ /(\d{4})-(\d\d)-(\d\d) (\d\d):(\d\d):(\d\d)/) {
					$last_change = timegm($6, $5, $4, $3, $2 - 1, $1);
				}
			}
			$info_cmd = "(cd $srcdir; $git_executable branch)";
			$line = qx{$info_cmd};
			if (defined($line)) {
				if ($line =~ /\* (\S+)/) {
					$repo_branch = $1;
				}
			}
			1;
			};
	}
	if ($num_commits == 0 and -d "$srcdir/.bzr") {

		# Try bzr...
		eval {
			use warnings "all";
			no warnings "all";
			$info_cmd = "(cd $srcdir; bzr log -l 1)";
			$line = qx{$info_cmd};
			if (defined($line)) {
				if ($line =~ /timestamp: \S+ (\d{4})-(\d\d)-(\d\d) (\d\d):(\d\d):(\d\d)/) {
					$last_change = timegm($6, $5, $4, $3, $2 - 1, $1);
				}
				if ($line =~ /svn revno: (\d+) \(on (\S+)\)/) {
					$num_commits = $1;
					$repo_branch = $2;
				}
				$vcs_name = "Bzr";
			}
			1;
			};
	}


	# 'svn info' failed or the user really wants us to dig around in .svn/entries
	if ($do_hack) {
		# Start of ugly internal SVN file hack
		if (! open (ENTRIES, "< $srcdir/.svn/entries")) {
			print STDERR "Unable to open $srcdir/.svn/entries\n";
		} else {
			$info_source = "Prodding .svn";
			# We need to find out whether our parser can handle the entries file
			$line = <ENTRIES>;
			chomp $line;
			if ($line eq '<?xml version="1.0" encoding="utf-8"?>') {
				$repo_version = "pre1.4";
			} elsif ($line =~ /^8$/) {
				$repo_version = "1.4";
			} else {
				$repo_version = "unknown";
			}

			if ($repo_version eq "pre1.4") {
				# The entries schema is flat, so we can use regexes to parse its contents.
				while ($line = <ENTRIES>) {
					if ($line =~ /<entry$/ || $line =~ /<entry\s/) {
						$in_entries = 1;
						$svn_name = "";
					}
					if ($in_entries) {
						if ($line =~ /name="(.*)"/) { $svn_name = $1; }
						if ($line =~ /committed-date="(\d{4})-(\d\d)-(\d\d)T(\d\d):(\d\d):(\d\d)/) {
							$last_change = timegm($6, $5, $4, $3, $2 - 1, $1);
						}
						if ($line =~ /revision="(\d+)"/) { $num_commits = $1; }
					}
					if ($line =~ /\/>/) {
						if (($svn_name eq "" || $svn_name eq "svn:this_dir") &&
								$last_change && $num_commits) {
							$in_entries = 0;
							last;
						}
					}
					# XXX - Fetch the repository root & URL
				}
			}
			close ENTRIES;
		}
	}

	# If we picked up the revision and modification time,
	# generate our strings.
	if ($version_pref{"pkg_enable"}) {
		$version_format =~ s/%#/$num_commits/;
		$package_format =~ s/%#/$num_commits-$commit_id/;
		$package_string = strftime($package_format, gmtime($last_change));
	}

	if ($get_vcs) {
		print <<"Fin";
Commit distance : $num_commits
Commit ID       : $commit_id
Revision source : $info_source
Release stamp   : $package_string
Fin
	} elsif ($print_vcs) {
		print new_version_h();
	}
}


# Read CMakeLists.txt, then write it back out with updated "set(PROJECT_..._VERSION ...)
# lines
# set(GIT_REVISION 999)
# set(PROJECT_MAJOR_VERSION 1)
# set(PROJECT_MINOR_VERSION 99)
# set(PROJECT_PATCH_VERSION 0)
# set(PROJECT_VERSION_EXTENSION "-rc5")
sub update_cmakelists_txt
{
	my $line;
	my $contents = "";
	my $version = "";
	my $filepath = "$srcdir/CMakeLists.txt";

	return if (!$set_version && $package_string eq "");

	open(CFGIN, "< $filepath") || die "Can't read $filepath!";
	while ($line = <CFGIN>) {
		if ($line =~ /^set *\( *GIT_REVISION .*([\r\n]+)$/) {
			$line = sprintf("set(GIT_REVISION %d)$1", $num_commits);
		} elsif ($line =~ /^set *\( *PROJECT_MAJOR_VERSION .*([\r\n]+)$/) {
			$line = sprintf("set(PROJECT_MAJOR_VERSION %d)$1", $version_pref{"version_major"});
		} elsif ($line =~ /^set *\( *PROJECT_MINOR_VERSION .*([\r\n]+)$/) {
			$line = sprintf("set(PROJECT_MINOR_VERSION %d)$1", $version_pref{"version_minor"});
		} elsif ($line =~ /^set *\( *PROJECT_PATCH_VERSION .*([\r\n]+)$/) {
			$line = sprintf("set(PROJECT_PATCH_VERSION %d)$1", $version_pref{"version_micro"});
		} elsif ($line =~ /^set *\( *PROJECT_VERSION_EXTENSION .*([\r\n]+)$/) {
			$line = sprintf("set(PROJECT_VERSION_EXTENSION \"%s\")$1", $package_string);
		}
		$contents .= $line
	}

	open(CFGIN, "> $filepath") || die "Can't write $filepath!";
	print(CFGIN $contents);
	close(CFGIN);
	print "$filepath has been updated.\n";
}

# Read configure.ac, then write it back out with an updated
# "AC_INIT" line.
sub update_configure_ac
{
	my $line;
	my $contents = "";
	my $version = "";
	my $filepath = "$srcdir/configure.ac";

	return if (!$set_version && $package_string eq "");

	open(CFGIN, "< $filepath") || die "Can't read $filepath!";
	while ($line = <CFGIN>) {
		if ($line =~ /^m4_define\( *\[?version_major\]? *,.*([\r\n]+)$/) {
			$line = sprintf("m4_define([version_major], [%d])$1", $version_pref{"version_major"});
		} elsif ($line =~ /^m4_define\( *\[?version_minor\]? *,.*([\r\n]+)$/) {
			$line = sprintf("m4_define([version_minor], [%d])$1", $version_pref{"version_minor"});
		} elsif ($line =~ /^m4_define\( *\[?version_micro\]? *,.*([\r\n]+)$/) {
			$line = sprintf("m4_define([version_micro], [%d])$1", $version_pref{"version_micro"});
		} elsif ($line =~ /^m4_define\( *\[?version_extra\]? *,.*([\r\n]+)$/) {
			$line = sprintf("m4_define([version_extra], [%s])$1", $package_string);
		}
		$contents .= $line
	}

	open(CFGIN, "> $filepath") || die "Can't write $filepath!";
	print(CFGIN $contents);
	close(CFGIN);
	print "$filepath has been updated.\n";
}

# Read docbook/asciidoc.conf, then write it back out with an updated
# wireshark-version replacement line.
sub update_release_notes
{
	my $line;
	my $contents = "";
	my $version = "";
	my $filepath = "$srcdir/docbook/asciidoc.conf";

	open(ADOC_CONF, "< $filepath") || die "Can't read $filepath!";
	while ($line = <ADOC_CONF>) {
		# wireshark-version:\[\]=1.9.1

		if ($line =~ /^wireshark-version=.*([\r\n]+)$/) {
			$line = sprintf("wireshark-version=%d.%d.%d$1",
					$version_pref{"version_major"},
					$version_pref{"version_minor"},
					$version_pref{"version_micro"},
				       );
		}
		$contents .= $line
	}

	open(ADOC_CONF, "> $filepath") || die "Can't write $filepath!";
	print(ADOC_CONF $contents);
	close(ADOC_CONF);
	print "$filepath has been updated.\n";
}

# Read debian/changelog, then write back out an updated version.
sub update_debian_changelog
{
	my $line;
	my $contents = "";
	my $version = "";
	my $filepath = "$srcdir/debian/changelog";

	open(CHANGELOG, "< $filepath") || die "Can't read $filepath!";
	while ($line = <CHANGELOG>) {
		if ($set_version && CHANGELOG->input_line_number() == 1) {
			$line = sprintf("wireshark (%d.%d.%d) unstable; urgency=low\n",
					$version_pref{"version_major"},
					$version_pref{"version_minor"},
					$version_pref{"version_micro"},
				       );
		}
		$contents .= $line
	}

	open(CHANGELOG, "> $filepath") || die "Can't write $filepath!";
	print(CHANGELOG $contents);
	close(CHANGELOG);
	print "$filepath has been updated.\n";
}

# Read Makefile.am for each library, then write back out an updated version.
sub update_automake_lib_releases
{
	my $line;
	my $contents = "";
	my $version = "";
	my $filedir;
	my $filepath;

	# The Libtool manual says
	#   "If the library source code has changed at all since the last
	#    update, then increment revision (‘c:r:a’ becomes ‘c:r+1:a’)."
	# epan changes with each minor release, almost by definition. wiretap
	# changes with *most* releases.
	#
	# http://www.gnu.org/software/libtool/manual/libtool.html#Updating-version-info
	for $filedir ("epan", "wiretap") {	# "wsutil"
		$contents = "";
		$filepath = $filedir . "/Makefile.am";
		open(MAKEFILE_AM, "< $filepath") || die "Can't read $filepath!";
		while ($line = <MAKEFILE_AM>) {
			# libwireshark_la_LDFLAGS = -version-info 2:1:1 -export-symbols

			if ($line =~ /^(lib\w+_la_LDFLAGS.*version-info\s+\d+:)\d+(:\d+.*)/) {
				$line = sprintf("$1%d$2\n", $version_pref{"version_micro"});
			}
			$contents .= $line
		}

		open(MAKEFILE_AM, "> $filepath") || die "Can't write $filepath!";
		print(MAKEFILE_AM $contents);
		close(MAKEFILE_AM);
		print "$filepath has been updated.\n";
	}
}

# Read CMakeLists.txt for each library, then write back out an updated version.
sub update_cmake_lib_releases
{
	my $line;
	my $contents = "";
	my $version = "";
	my $filedir;
	my $filepath;

	for $filedir ("epan", "wiretap") {	# "wsutil"
		$contents = "";
		$filepath = $filedir . "/CMakeLists.txt";
		open(CMAKELISTS_TXT, "< $filepath") || die "Can't read $filepath!";
		while ($line = <CMAKELISTS_TXT>) {
			# set(FULL_SO_VERSION "0.0.0")

			if ($line =~ /^(set\s*\(\s*FULL_SO_VERSION\s+"\d+\.\d+\.)\d+(".*)/) {
				$line = sprintf("$1%d$2\n", $version_pref{"version_micro"});
			}
			$contents .= $line
		}

		open(CMAKELISTS_TXT, "> $filepath") || die "Can't write $filepath!";
		print(CMAKELISTS_TXT $contents);
		close(CMAKELISTS_TXT);
		print "$filepath has been updated.\n";
	}
}

# Update distributed files that contain any version information
sub update_versioned_files
{
        # Matches CMakeLists.txt
        printf "GR: %d, MaV: %d, MiV: %d, PL: %d, EV: %s\n",
                $num_commits, $version_pref{"version_major"},
                $version_pref{"version_minor"}, $version_pref{"version_micro"},
                $package_string;
	&update_cmakelists_txt;
	&update_configure_ac;
	if ($set_version) {
		&update_release_notes;
		&update_debian_changelog;
		&update_automake_lib_releases;
		&update_cmake_lib_releases;
	}
}

sub new_version_h
{
	if (!$enable_vcsversion) {
		return "/* #undef VCSVERSION */\n";
	}

	if ($git_description) {
		# Do not bother adding the git branch, the git describe output
		# normally contains the base tag and commit ID which is more
		# than sufficient to determine the actual source tree.
		return "#define VCSVERSION \"$git_description\"\n";
	}

	if ($last_change && $num_commits) {
		return "#define VCSVERSION \"$vcs_name Rev $num_commits from $repo_branch\"\n";
	}

	return "#define VCSVERSION \"$vcs_name Rev Unknown from unknown\"\n";
}

# Print the version control system's version to $version_file.
# Don't change the file if it is not needed.
sub print_VCS_REVISION
{
	my $VCS_REVISION;
	my $needs_update = 1;

	$VCS_REVISION = new_version_h();
	if (open(OLDREV, "<$version_file")) {
		my $old_VCS_REVISION = <OLDREV>;
		if ($old_VCS_REVISION eq $VCS_REVISION) {
			$needs_update = 0;
		}
		close OLDREV;
	}

	if (! $set_vcs) { return; }

	if ($needs_update) {
		# print "Updating $version_file so it contains:\n$VCS_REVISION";
		open(VER, ">$version_file") || die ("Cannot write to $version_file ($!)\n");
		print VER "$VCS_REVISION";
		close VER;
		print "$version_file has been updated.\n";
	} elsif (!$enable_vcsversion) {
		print "$version_file disabled.\n";
	} else {
		print "$version_file unchanged.\n";
	}
}

# Read values from the configuration file, if it exists.
sub get_config {
	my $arg;
	my $show_help = 0;

	# Get our command-line args
	# XXX - Do we need an option to undo --set-release?
	GetOptions(
		   "help|h", \$show_help,
		   "get-vcs|get-svn|g", \$get_vcs,
		   "set-vcs|set-svn|s", \$set_vcs,
		   "git-bin", \$git_executable,
		   "print-vcs", \$print_vcs,
		   "set-version|v", \$set_version,
		   "set-release|r|package-version|p", \$set_release,
		   "verbose", \$verbose
		   ) || pod2usage(2);

	if ($show_help) { pod2usage(1); }

	if ( !( $show_help || $get_vcs || $set_vcs || $print_vcs || $set_version || $set_release ) ) {
		$set_vcs = 1;
	}

	if ($#ARGV >= 0) {
		$srcdir = $ARGV[0]
	}

	if (! open(FILE, "<$vconf_file")) {
		if (! open(FILE, "<$srcdir/$vconf_file")) {
			print_diag "Version configuration file $vconf_file not "
			. "found. Using defaults.\n";
			return 1;
		}
	}

	while (<FILE>) {
		s/^\s+|\s+$//g; # chomp() may not handle CR
		next if (/^#/);
		next unless (/^(\w+)(:|=)\s*(\S.*)/);
		$version_pref{$1} = $3;
	}
	close FILE;
	return 1;
}

##
## Start of code
##

&get_config();

&read_repo_info();

&print_VCS_REVISION;

if ($set_version || $set_release) {
	if ($set_version) {
		print "Generating version information.\n";
	}

	if ($version_pref{"enable"} == 0) {
		print "Release information disabled in $vconf_file.\n";
		$set_release = 0;
	}

	if ($set_release) {
		print "Generating release information.\n";
	} else {
		print "Resetting release information\n";
		$num_commits = 0;
		$package_string = "";
	}

	&update_versioned_files;
}

__END__

=head1 NAM

make-version.pl - Get and set build-time version information for Wireshark

=head1 SYNOPSIS

make-version.pl [options] [source directory]

  Options:

    --help, -h                 This help message
    --get-vcs, -g              Print the VCS revision and source.
    --set-vcs, -s              Set the information in version.h
    --print-vcs                Print the vcs version to standard output
    --set-version, -v          Set the major, minor, and micro versions in
                               the top-level CMakeLists.txt, configure.ac,
                               docbook/asciidoc.conf, debian/changelog,
                               the Makefile.am for all libraries, and the
                               CMakeLists.txt for all libraries.
                               Resets the release information when used by
                               itself.
    --set-release, -r          Set the release information in the top-level
                               CMakeLists.txt, configure.ac
    --package-version, -p      Deprecated. Same as --set-release.
    --verbose                  Print diagnostic messages to STDERR.

Options can be used in any combination. If none are specified B<--set-vcs>
is assumed.

=cut

#
# Editor modelines  -  http://www.wireshark.org/tools/modelines.html
#
# Local variables:
# c-basic-offset: 8
# tab-width: 8
# indent-tabs-mode: t
# End:
#
# vi: set shiftwidth=8 tabstop=8 noexpandtab:
# :indentSize=8:tabSize=8:noTabs=false:
#
#
