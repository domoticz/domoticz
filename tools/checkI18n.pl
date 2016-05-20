#!/usr/bin/perl

use strict;
use English;
use warnings;

use FileHandle;
use HTML::TagParser;
use JSON::Parse;
use File::Slurp;

# Maps translation to the HTML file it was used in
my %translations;


##
# Find all required translations in an HTML node
# @param node the node to parse
# @param file the name of the file the node is in
# @post %translations has keys set for each translation
sub findRequiredTranslationsInNode {
	my ($node, $file) = @ARG;

	my $translation = $node->getAttribute('data-i18n');
	if (defined $translation) {
		$translations{$translation} = $file;
#		warn "Found translation '$translation'\n";
	}

	my @children = @{$node->childNodes()};

	foreach my $child (@children) {
		findRequiredTranslationsInNode($child, $file);
	}
}

##
# Find all required translations in a file
# @param file the filename
# @post %translations has keys set for each translation
sub findRequiredTranslationsInHTMLFile {
	my ($file) = @ARG;

	my $html = HTML::TagParser->new($file);

	my $rootNode;

	my @nodes;
	foreach my $tag ('html', 'body', 'div') {
		if ((@nodes = $html->getElementsByTagName($tag)) > 0) {
			last;
		}
	}

	return unless @nodes > 0;

	foreach my $node (@nodes) {
		findRequiredTranslationsInNode($node, $file);
	}
}

##
# Find all required translations in a directory
# @param dir the directory path
# @post %translations has keys set for each translation
sub findRequiredTranslationsInDir {
	my ($dir) = @ARG;

	my @files = glob "$dir/*.html";
	foreach my $file (@files) {
		findRequiredTranslationsInHTMLFile($file);
	}

	@files = glob "$dir/*.html $dir/*.js";
	foreach my $file (@files) {
		findRequiredTranslationsInJavascriptFile($file);
	}

	foreach my $childDir (grep { -d "$dir/$ARG" } read_dir($dir)) {
		findRequiredTranslationsInDir("$dir/$childDir");
	}
}

##
# Find all required translations in a javascript file
# @param file the filename to check
# @post %translations has keys set for each translation
sub findRequiredTranslationsInJavascriptFile {
	my ($file) = @ARG;

	warn "parsing Javascript '$file'";

	my $contents = read_file($file) or
		die "Could not read '$file': $OS_ERROR";

	while ($contents =~ /\$\.t\('(.*?)'\)/g) {
		warn "Found js translation in '$file': '$1'";
		$translations{$1} = $file;
	}
}

##
# Find bad translations in a json file
# @param file the filename tho check
# @return true if there were no missing translations, false otherwise
sub checkForBadTranslationsInJsonFile {
	my ($file) = @ARG;

	my %json = %{JSON::Parse::json_file_to_perl ($file)};

	my $ret = 1;
	foreach my $translation (sort keys %translations) {
		if (!defined $json{$translation}) {
			warn "No translation for '$translation', used in " . $translations{$translation} . ", missing in '$file'\n";
			$ret = 0;
		}
	}

	foreach my $translation (sort keys %json) {
		if (!defined $translations{$translation}) {
			warn "Unused translation for '$translation', defined in '$file'\n";
			$ret = 0;
		}
	}
}

##
# Find Bad translations in a directory
# @param dir the directory path
# @return true if there were no bad translations, false otherwise
sub checkForBadTranslationsInJsonDir {
	my ($dir) = @ARG;

	my @files = glob "$dir/*.json";

	my $ret = 1;

	foreach my $file (@files) {
		$ret &= checkForBadTranslationsInJsonFile($file);
	}
}



findRequiredTranslationsInDir('www');
checkForBadTranslationsInJsonDir('i18n') ||
	die "Missing/unused translations found\n";

exit 0;