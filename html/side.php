<?php
include_once(dirname(__FILE__).'/includes/utils.inc.php');
header('Cache-Control: no-store');
header('Pragma: no-cache');

$this_version = '4.5.13';
$link_target = 'main';
$theme = isset($cfg['theme']) ? $cfg['theme'] : 'dark';
if ($theme !== 'dark' && $theme !== 'light') {
	$theme = 'dark';
}
$cgi_base_url = htmlspecialchars($cfg['cgi_base_url'], ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
?>
<!doctype html>
<html id="side" lang="en" class="<?php echo $theme; ?>">
<head>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1">
	<meta name="robots" content="noindex, nofollow">
	<meta name="referrer" content="same-origin">
	<title>Nagios Core navigation</title>
	<link rel="stylesheet" href="stylesheets/common.css?<?php echo $this_version; ?>">
	<link rel="stylesheet" href="stylesheets/frontend.css?<?php echo $this_version; ?>">
	<link rel="stylesheet" href="stylesheets/theme.css?<?php echo $this_version; ?>">
	<script src="js/coreui.js?<?php echo $this_version; ?>" defer></script>
</head>
<body class="navbar">
	<a class="navbar-brand" href="main.php" target="<?php echo $link_target; ?>" aria-label="Nagios Core overview">
		<span class="fulllogo nagioslogo" aria-hidden="true"></span>
	</a>

	<nav class="primary-nav" aria-label="Primary">
		<section class="navsection" aria-labelledby="nav-overview">
			<h2 class="navsectiontitle" id="nav-overview">Overview</h2>
			<ul class="navsectionlinks">
				<li><a href="main.php" target="<?php echo $link_target; ?>">Dashboard</a></li>
				<li><a href="<?php echo $cgi_base_url; ?>/tac.cgi" target="<?php echo $link_target; ?>">Tactical overview</a></li>
				<li><a href="<?php echo $cgi_base_url; ?>/statusmap.cgi?host=all" target="<?php echo $link_target; ?>">Network map</a></li>
			</ul>
		</section>

		<section class="navsection" aria-labelledby="nav-status">
			<h2 class="navsectiontitle" id="nav-status">Current status</h2>
			<ul class="navsectionlinks">
				<li><a href="<?php echo $cgi_base_url; ?>/status.cgi?hostgroup=all&amp;style=hostdetail" target="<?php echo $link_target; ?>">Hosts</a></li>
				<li><a href="<?php echo $cgi_base_url; ?>/status.cgi?host=all" target="<?php echo $link_target; ?>">Services</a></li>
				<li>
					<span class="nav-group-label">Host groups</span>
					<ul>
						<li><a href="<?php echo $cgi_base_url; ?>/status.cgi?hostgroup=all&amp;style=overview" target="<?php echo $link_target; ?>">Overview</a></li>
						<li><a href="<?php echo $cgi_base_url; ?>/status.cgi?hostgroup=all&amp;style=summary" target="<?php echo $link_target; ?>">Summary</a></li>
						<li><a href="<?php echo $cgi_base_url; ?>/status.cgi?hostgroup=all&amp;style=grid" target="<?php echo $link_target; ?>">Grid</a></li>
					</ul>
				</li>
				<li>
					<span class="nav-group-label">Service groups</span>
					<ul>
						<li><a href="<?php echo $cgi_base_url; ?>/status.cgi?servicegroup=all&amp;style=overview" target="<?php echo $link_target; ?>">Overview</a></li>
						<li><a href="<?php echo $cgi_base_url; ?>/status.cgi?servicegroup=all&amp;style=summary" target="<?php echo $link_target; ?>">Summary</a></li>
						<li><a href="<?php echo $cgi_base_url; ?>/status.cgi?servicegroup=all&amp;style=grid" target="<?php echo $link_target; ?>">Grid</a></li>
					</ul>
				</li>
			</ul>
		</section>

		<section class="navsection" aria-labelledby="nav-problems">
			<h2 class="navsectiontitle" id="nav-problems">Problems</h2>
			<ul class="navsectionlinks">
				<li class="nav-link-row">
					<a href="<?php echo $cgi_base_url; ?>/status.cgi?host=all&amp;servicestatustypes=28" target="<?php echo $link_target; ?>">Service problems</a>
					<a class="nav-chip" href="<?php echo $cgi_base_url; ?>/status.cgi?host=all&amp;type=detail&amp;hoststatustypes=3&amp;serviceprops=10&amp;servicestatustypes=28" target="<?php echo $link_target; ?>">Unhandled</a>
				</li>
				<li class="nav-link-row">
					<a href="<?php echo $cgi_base_url; ?>/status.cgi?hostgroup=all&amp;style=hostdetail&amp;hoststatustypes=12" target="<?php echo $link_target; ?>">Host problems</a>
					<a class="nav-chip" href="<?php echo $cgi_base_url; ?>/status.cgi?hostgroup=all&amp;style=hostdetail&amp;hoststatustypes=12&amp;hostprops=42" target="<?php echo $link_target; ?>">Unhandled</a>
				</li>
				<li><a href="<?php echo $cgi_base_url; ?>/outages.cgi" target="<?php echo $link_target; ?>">Network outages</a></li>
			</ul>
		</section>

		<section class="navsection" aria-labelledby="nav-reports">
			<h2 class="navsectiontitle" id="nav-reports">Reports</h2>
			<ul class="navsectionlinks">
				<li><a href="<?php echo $cgi_base_url; ?>/avail.cgi" target="<?php echo $link_target; ?>">Availability</a></li>
				<li><a href="<?php echo $cgi_base_url; ?>/trends.cgi" target="<?php echo $link_target; ?>">Trends</a></li>
				<li>
					<span class="nav-group-label">Alerts</span>
					<ul>
						<li><a href="<?php echo $cgi_base_url; ?>/history.cgi?host=all" target="<?php echo $link_target; ?>">History</a></li>
						<li><a href="<?php echo $cgi_base_url; ?>/summary.cgi" target="<?php echo $link_target; ?>">Summary</a></li>
						<li><a href="<?php echo $cgi_base_url; ?>/histogram.cgi" target="<?php echo $link_target; ?>">Histogram</a></li>
					</ul>
				</li>
				<li><a href="<?php echo $cgi_base_url; ?>/notifications.cgi?contact=all" target="<?php echo $link_target; ?>">Notifications</a></li>
				<li><a href="<?php echo $cgi_base_url; ?>/showlog.cgi" target="<?php echo $link_target; ?>">Event log</a></li>
			</ul>
		</section>

		<section class="navsection" aria-labelledby="nav-system">
			<h2 class="navsectiontitle" id="nav-system">System</h2>
			<ul class="navsectionlinks">
				<li><a href="<?php echo $cgi_base_url; ?>/extinfo.cgi?type=3" target="<?php echo $link_target; ?>">Comments</a></li>
				<li><a href="<?php echo $cgi_base_url; ?>/extinfo.cgi?type=6" target="<?php echo $link_target; ?>">Downtime</a></li>
				<li><a href="<?php echo $cgi_base_url; ?>/extinfo.cgi?type=0" target="<?php echo $link_target; ?>">Process information</a></li>
				<li><a href="<?php echo $cgi_base_url; ?>/extinfo.cgi?type=4" target="<?php echo $link_target; ?>">Performance</a></li>
				<li><a href="<?php echo $cgi_base_url; ?>/extinfo.cgi?type=7" target="<?php echo $link_target; ?>">Scheduling queue</a></li>
				<li><a href="<?php echo $cgi_base_url; ?>/config.cgi" target="<?php echo $link_target; ?>">Configuration</a></li>
			</ul>
		</section>
	</nav>

	<form class="navbarsearch" method="get" action="<?php echo $cgi_base_url; ?>/status.cgi" target="<?php echo $link_target; ?>" role="search">
		<label for="navbar-host-search">Quick search</label>
		<input type="hidden" name="navbarsearch" value="1">
		<input id="navbar-host-search" type="search" name="host" placeholder="Host or service" autocomplete="off">
	</form>

	<footer class="navbar-footer">
		<a href="https://assets.nagios.com/downloads/nagioscore/docs/nagioscore/4/en/" target="_blank" rel="noopener noreferrer">Documentation</a>
		<span aria-hidden="true">·</span>
		<a href="https://www.nagios.org" target="_blank" rel="noopener noreferrer">nagios.org</a>
	</footer>
</body>
</html>
