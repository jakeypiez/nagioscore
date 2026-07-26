#!/usr/bin/perl

use warnings;
use strict;
use Test::More tests => 38;

defined($ARGV[0]) or die "Usage: $0 <top build dir>";

my $top_builddir = shift @ARGV;
my $cmd_cgi = "$top_builddir/cgi/cmd.cgi";
my $statuswml_cgi = "$top_builddir/cgi/statuswml.cgi";
my $base_env = 'NAGIOS_CGI_CONFIG=etc/cgi.cfg REMOTE_USER=nagiosadmin';

sub post_command {
	my ($body, $cookie, $remote_user) = @_;
	my $content_length = length($body);
	return `printf '%s' '$body' | NAGIOS_CGI_CONFIG=etc/cgi.cfg REMOTE_USER='$remote_user' HTTP_COOKIE='NagFormId=$cookie' REQUEST_METHOD=POST CONTENT_TYPE=application/x-www-form-urlencoded CONTENT_LENGTH=$content_length '$cmd_cgi'`;
}

my $first = `$base_env REQUEST_METHOD=GET QUERY_STRING='cmd_typ=1' $cmd_cgi`;
my ($first_token) = $first =~ /Set-Cookie: NagFormId=([0-9a-f]{64}); Path=\/; HttpOnly; SameSite=Strict\r?/;
ok(defined($first_token), 'command form sets a 256-bit hexadecimal CSRF token');
like($first, qr/^Cache-Control: no-store\r?$/m, 'command form is not cached');
unlike($first, qr/; Secure\r?$/m, 'development HTTP cookie is not marked Secure');

my $second = `$base_env REQUEST_METHOD=GET QUERY_STRING='cmd_typ=1' $cmd_cgi`;
my ($second_token) = $second =~ /Set-Cookie: NagFormId=([0-9a-f]{64});/;
ok(defined($second_token), 'a second command form gets a valid token');
isnt($second_token, $first_token, 'independent CGI processes receive different tokens');

my $https = `$base_env HTTPS=on SCRIPT_NAME='/custom/cgi-bin/cmd.cgi' REQUEST_METHOD=GET QUERY_STRING='cmd_typ=1' $cmd_cgi`;
like(
	$https,
	qr/Set-Cookie: __Host-NagFormId=[0-9a-f]{64}; Path=\/; HttpOnly; SameSite=Strict; Secure\r?/,
	'HTTPS uses a host-only, HttpOnly, strict same-site, and Secure cookie'
);
my ($https_token) = $https =~ /Set-Cookie: __Host-NagFormId=([0-9a-f]{64});/;
$https_token = '0' x 64 unless defined($https_token);
my $https_reused = `$base_env HTTPS=on HTTP_COOKIE='__Host-NagFormId=$https_token' REQUEST_METHOD=GET QUERY_STRING='cmd_typ=1' $cmd_cgi`;
unlike($https_reused, qr/^Set-Cookie:/m, 'a valid __Host- token is reused over HTTPS');
like($https_reused, qr/NAME='nagFormId' VALUE='\Q$https_token\E'/, 'the secure cookie token is copied into the form');

my $https_legacy_cookie = `$base_env HTTPS=on HTTP_COOKIE='NagFormId=$first_token' REQUEST_METHOD=GET QUERY_STRING='cmd_typ=1' $cmd_cgi`;
like($https_legacy_cookie, qr/^Set-Cookie: __Host-NagFormId=[0-9a-f]{64};/m, 'HTTPS ignores a sibling-settable legacy cookie');
unlike($https_legacy_cookie, qr/NAME='nagFormId' VALUE='\Q$first_token\E'/, 'a legacy cookie cannot choose the HTTPS form token');

my $reused = `$base_env HTTP_COOKIE='NagFormId=$first_token' REQUEST_METHOD=GET QUERY_STRING='cmd_typ=1' $cmd_cgi`;
unlike($reused, qr/^Set-Cookie:/m, 'a valid existing token is reused');
like(
	$reused,
	qr/<INPUT TYPE='hidden' NAME='nagFormId' VALUE='\Q$first_token\E'>/,
	'the matching token is placed in the POST form'
);

my $legacy = `$base_env HTTP_COOKIE='NagFormId=deadbeef' REQUEST_METHOD=GET QUERY_STRING='cmd_typ=1' $cmd_cgi`;
like($legacy, qr/Set-Cookie: NagFormId=[0-9a-f]{64};/, 'short legacy tokens are replaced');
unlike($legacy, qr/NAME='nagFormId' VALUE='deadbeef'/, 'short legacy tokens are never trusted');

my $attacker_token = 'a' x 64;
my $encoded_name = `$base_env REQUEST_METHOD=GET QUERY_STRING='cmd_typ=1&N%61gFormId=$attacker_token' $cmd_cgi`;
like($encoded_name, qr/Set-Cookie: NagFormId=[0-9a-f]{64};/, 'percent-encoded cookie parameter names are ignored');
unlike($encoded_name, qr/NAME='nagFormId' VALUE='$attacker_token'/, 'an encoded query parameter cannot populate the trusted hidden field');

my $trailing_percent = `$base_env REQUEST_METHOD=GET QUERY_STRING='cmd_typ=1&name%=value' $cmd_cgi`;
like($trailing_percent, qr/Set-Cookie: NagFormId=[0-9a-f]{64};/, 'a parameter name ending in percent is handled safely');
my $short_escape = `$base_env REQUEST_METHOD=GET QUERY_STRING='cmd_typ=1&name%6=value' $cmd_cgi`;
like($short_escape, qr/Set-Cookie: NagFormId=[0-9a-f]{64};/, 'an incomplete parameter-name escape is handled safely');

my $smuggled_cookie = `$base_env HTTP_COOKIE='other=value NagFormId=$first_token' REQUEST_METHOD=GET QUERY_STRING='cmd_typ=1' $cmd_cgi`;
like($smuggled_cookie, qr/^Set-Cookie: NagFormId=[0-9a-f]{64};/m, 'cookie text embedded in another value is rejected');

my $many_query = join('&', 'cmd_typ=1', map { "parameter$_=value" } 1..254);
my $many_parameters = `$base_env HTTP_COOKIE='NagFormId=$first_token' REQUEST_METHOD=GET QUERY_STRING='$many_query' $cmd_cgi`;
unlike($many_parameters, qr/^Set-Cookie:/m, 'a valid cookie survives a full parameter allocation boundary');
like($many_parameters, qr/NAME='nagFormId' VALUE='\Q$first_token\E'/, 'cookie append retains room for the terminating parameter entry');

my $wrong_token = '0' x 64;
my $wrong = `$base_env HTTP_COOKIE='NagFormId=$first_token' REQUEST_METHOD=GET QUERY_STRING='cmd_typ=1&cmd_mod=2&nagFormId=$wrong_token' $cmd_cgi`;
like($wrong, qr/Error: Command submissions require POST\./, 'GET command commit is rejected before token validation');
unlike($wrong, qr/Could not open command file for update/, 'rejected GET does not reach the command pipe');

my $valid_get = `$base_env HTTP_COOKIE='NagFormId=$first_token' REQUEST_METHOD=GET QUERY_STRING='cmd_typ=1&cmd_mod=2&nagFormId=$first_token' $cmd_cgi`;
like($valid_get, qr/Error: Command submissions require POST\./, 'even a valid token cannot authorize a GET commit');

my $wrong_post_body = "cmd_typ=12&cmd_mod=2&nagFormId=$wrong_token";
my $wrong_post = post_command($wrong_post_body, $first_token, 'nagiosadmin');
like($wrong_post, qr/Error: Invalid or missing CSRF cookie!/, 'a POST with a mismatched token is rejected');
unlike($wrong_post, qr/not authorized to commit/, 'a rejected POST does not reach command authorization');

my $valid_post_body = "cmd_typ=12&cmd_mod=2&nagFormId=$first_token";
my $valid_post = post_command($valid_post_body, $first_token, 'csrf-test-user');
unlike($valid_post, qr/Error: Invalid or missing CSRF cookie!/, 'a matching cookie and form token pass CSRF validation');
like($valid_post, qr/not authorized to commit/, 'a valid POST reaches command authorization without executing as an unauthorized user');

my $cross_origin_length = length($valid_post_body);
my $cross_origin = `printf '%s' '$valid_post_body' | NAGIOS_CGI_CONFIG=etc/cgi.cfg REMOTE_USER='nagiosadmin' HTTP_COOKIE='NagFormId=$first_token' HTTP_HOST='nagios.example' HTTP_ORIGIN='http://evil.example' REQUEST_METHOD=POST CONTENT_TYPE=application/x-www-form-urlencoded CONTENT_LENGTH=$cross_origin_length '$cmd_cgi'`;
like($cross_origin, qr/origin did not match this Nagios server/, 'a browser POST from another origin is rejected');
unlike($cross_origin, qr/not authorized to commit|successfully submitted/, 'origin rejection occurs before authorization or command execution');

my $same_origin_http = `printf '%s' '$valid_post_body' | NAGIOS_CGI_CONFIG=etc/cgi.cfg REMOTE_USER='csrf-test-user' HTTP_COOKIE='NagFormId=$first_token' HTTP_HOST='nagios.example:80' HTTP_ORIGIN='http://nagios.example' REQUEST_METHOD=POST CONTENT_TYPE=application/x-www-form-urlencoded CONTENT_LENGTH=$cross_origin_length '$cmd_cgi'`;
unlike($same_origin_http, qr/origin did not match this Nagios server/, 'an HTTP same-origin POST accepts an omitted default port');
like($same_origin_http, qr/not authorized to commit/, 'a valid HTTP same-origin POST reaches authorization');

my $same_origin_https_body = "cmd_typ=12&cmd_mod=2&nagFormId=$https_token";
my $same_origin_https_length = length($same_origin_https_body);
my $same_origin_https = `printf '%s' '$same_origin_https_body' | NAGIOS_CGI_CONFIG=etc/cgi.cfg REMOTE_USER='csrf-test-user' HTTPS=on HTTP_COOKIE='__Host-NagFormId=$https_token' HTTP_HOST='nagios.example:443' HTTP_ORIGIN='https://nagios.example' REQUEST_METHOD=POST CONTENT_TYPE=application/x-www-form-urlencoded CONTENT_LENGTH=$same_origin_https_length '$cmd_cgi'`;
unlike($same_origin_https, qr/origin did not match this Nagios server/, 'an HTTPS same-origin POST accepts an omitted default port');
like($same_origin_https, qr/not authorized to commit/, 'a valid HTTPS same-origin POST reaches authorization');

my $wml = `$base_env SCRIPT_NAME='/nagios/cgi-bin/statuswml.cgi' REQUEST_METHOD=GET QUERY_STRING='style=processinfo' '$statuswml_cgi'`;
my ($wml_token) = $wml =~ /Set-Cookie: NagFormId=([0-9a-f]{64});/;
ok(defined($wml_token), 'the WML interface receives a secure command-form token');
$wml_token = '0' x 64 unless defined($wml_token);
like($wml, qr/postfield name='nagFormId' value='\Q$wml_token\E'/, 'WML command links submit the matching form token');
my $wml_reused = `$base_env SCRIPT_NAME='/nagios/cgi-bin/statuswml.cgi' HTTP_COOKIE='NagFormId=$wml_token' REQUEST_METHOD=GET QUERY_STRING='style=processinfo' '$statuswml_cgi'`;
unlike($wml_reused, qr/^Set-Cookie:/m, 'WML navigation reuses an existing token without invalidating open command cards');
like($wml_reused, qr/postfield name='nagFormId' value='\Q$wml_token\E'/, 'reused WML forms retain the cookie token');
