<?php
include_once(dirname(__FILE__).'/includes/utils.inc.php');
header('Cache-Control: no-store');
header('Pragma: no-cache');

$this_version = '4.5.13';
$asset_version = $this_version . '-modern-ui-2';
$this_year = '2026';
$release_date = 'May 28, 2026';
$theme = isset($cfg['theme']) ? $cfg['theme'] : 'dark';
if ($theme !== 'dark' && $theme !== 'light') {
	$theme = 'dark';
}

$cgi_base_url = htmlspecialchars($cfg['cgi_base_url'], ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
$tour_enabled = !empty($cfg['enable_page_tour']) ? '1' : '0';
$tour_user = htmlspecialchars(isset($_SERVER['REMOTE_USER']) ? $_SERVER['REMOTE_USER'] : '', ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
$status_update_interval = 10;
if (isset($cfg['main_config_file']) && is_readable($cfg['main_config_file'])) {
	$config_lines = @file($cfg['main_config_file'], FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
	if (is_array($config_lines)) {
		foreach ($config_lines as $config_line) {
			if (preg_match('/^\s*status_update_interval\s*=\s*([0-9]+)\s*(?:#.*)?$/', $config_line, $matches)) {
				$status_update_interval = max(1, (int)$matches[1]);
				break;
			}
		}
	}
}
$maximum_status_age = max(120, ($status_update_interval * 2) + 30);
?>
<!doctype html>
<html id="main" lang="en" class="<?php echo $theme; ?>">
<head>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1">
	<meta name="robots" content="noindex, nofollow">
	<meta name="referrer" content="same-origin">
	<title>Nagios Core monitoring overview</title>
	<link rel="stylesheet" href="stylesheets/common.css?<?php echo $asset_version; ?>">
	<link rel="stylesheet" href="stylesheets/nag_funcs.css?<?php echo $asset_version; ?>">
	<link rel="stylesheet" href="stylesheets/frontend.css?<?php echo $asset_version; ?>">
	<link rel="stylesheet" href="stylesheets/theme.css?<?php echo $asset_version; ?>">
	<script src="js/jquery-3.7.1.min.js" defer></script>
	<script src="js/nag_funcs.js?<?php echo $asset_version; ?>" defer></script>
	<script src="js/coreui.js?<?php echo $asset_version; ?>" defer></script>
	<script src="js/dashboard.js?<?php echo $asset_version; ?>" defer></script>
</head>
<body id="splashpage" class="dashboard" data-api-base="<?php echo $cgi_base_url; ?>" data-status-max-age="<?php echo $maximum_status_age; ?>" data-tour-enabled="<?php echo $tour_enabled; ?>" data-tour-user="<?php echo $tour_user; ?>">
	<header class="dashboard-header">
		<div>
			<p class="eyebrow">Nagios Core <?php echo $this_version; ?></p>
			<h1>Monitoring overview</h1>
			<p class="dashboard-intro">A live view of the monitoring engine, hosts, and services in this installation.</p>
		</div>
		<div class="version-panel" aria-label="Installed version">
			<span>Version <strong><?php echo $this_version; ?></strong></span>
			<span>Released <?php echo $release_date; ?></span>
			<a href="https://www.nagios.org/checkforupdates/?version=<?php echo $this_version; ?>&amp;product=nagioscore" target="_blank" rel="noopener noreferrer">Check for updates</a>
		</div>
	</header>

	<div id="updateversioninfo" aria-live="polite">
	<?php
	$updateinfo = get_update_information();
	if (!$updateinfo['update_checks_enabled']) {
	?>
		<div class="updatechecksdisabled" role="status">
			<strong>Automatic update checks are disabled.</strong>
			<span>Check <a href="https://www.nagios.org/" target="_blank" rel="noopener noreferrer">nagios.org</a> manually or enable update checks in the Nagios configuration.</span>
		</div>
	<?php
	} elseif ($updateinfo['update_available'] && version_compare($this_version, $updateinfo['update_version'], '<')) {
	?>
		<div class="updateavailable" role="status">
			<strong>Nagios Core <?php echo htmlentities($updateinfo['update_version'], ENT_QUOTES, 'UTF-8'); ?> is available.</strong>
			<a href="https://www.nagios.org/download/" target="_blank" rel="noopener noreferrer">View the release</a>
		</div>
	<?php
	}
	?>
	</div>

	<section class="metric-grid" aria-label="Current monitoring status" aria-live="polite">
		<article class="metric-card metric-card-engine">
			<div class="metric-heading">
				<span class="metric-icon engine-icon" aria-hidden="true"></span>
				<span>Core engine</span>
			</div>
			<p class="metric-value metric-status" id="core-status"><span class="status-dot is-loading" aria-hidden="true"></span>Checking status…</p>
			<p class="metric-detail" id="core-detail">Connecting to the local status API</p>
			<a class="metric-link" href="<?php echo $cgi_base_url; ?>/extinfo.cgi?type=0">Process information <span aria-hidden="true">→</span></a>
		</article>

		<article class="metric-card">
			<div class="metric-heading">
				<span class="metric-icon host-icon" aria-hidden="true"></span>
				<span>Hosts</span>
			</div>
			<p class="metric-value" id="host-total">—</p>
			<ul class="state-summary" id="host-summary" aria-label="Host state totals">
				<li><span class="state-dot state-up" aria-hidden="true"></span><span data-host-state="up">—</span> up</li>
				<li><span class="state-dot state-down" aria-hidden="true"></span><span data-host-state="down">—</span> down</li>
				<li><span class="state-dot state-unknown" aria-hidden="true"></span><span data-host-state="unreachable">—</span> unreachable</li>
				<li><span class="state-dot state-pending" aria-hidden="true"></span><span data-host-state="pending">—</span> pending</li>
			</ul>
			<a class="metric-link" href="<?php echo $cgi_base_url; ?>/status.cgi?hostgroup=all&amp;style=hostdetail">View all hosts <span aria-hidden="true">→</span></a>
		</article>

		<article class="metric-card">
			<div class="metric-heading">
				<span class="metric-icon service-icon" aria-hidden="true"></span>
				<span>Services</span>
			</div>
			<p class="metric-value" id="service-total">—</p>
			<ul class="state-summary" id="service-summary" aria-label="Service state totals">
				<li><span class="state-dot state-up" aria-hidden="true"></span><span data-service-state="ok">—</span> ok</li>
				<li><span class="state-dot state-warning" aria-hidden="true"></span><span data-service-state="warning">—</span> warning</li>
				<li><span class="state-dot state-down" aria-hidden="true"></span><span data-service-state="critical">—</span> critical</li>
				<li><span class="state-dot state-unknown" aria-hidden="true"></span><span data-service-state="unknown">—</span> unknown</li>
				<li><span class="state-dot state-pending" aria-hidden="true"></span><span data-service-state="pending">—</span> pending</li>
			</ul>
			<a class="metric-link" href="<?php echo $cgi_base_url; ?>/status.cgi?host=all">View all services <span aria-hidden="true">→</span></a>
		</article>
	</section>

	<section class="quick-actions" aria-labelledby="quick-actions-title">
		<div class="section-heading">
			<div>
				<p class="eyebrow">Workspace</p>
				<h2 id="quick-actions-title">Common tasks</h2>
			</div>
			<a href="https://assets.nagios.com/downloads/nagioscore/docs/nagioscore/4/en/" target="_blank" rel="noopener noreferrer">Open documentation <span aria-hidden="true">↗</span></a>
		</div>
		<div class="action-grid">
			<a class="action-card" href="<?php echo $cgi_base_url; ?>/tac.cgi"><strong>Tactical overview</strong><span>Review health, outages, and monitoring features.</span></a>
			<a class="action-card" href="<?php echo $cgi_base_url; ?>/status.cgi?host=all&amp;servicestatustypes=28"><strong>Service problems</strong><span>Focus on warning, unknown, and critical services.</span></a>
			<a class="action-card" href="<?php echo $cgi_base_url; ?>/statusmap.cgi?host=all"><strong>Network map</strong><span>Explore parent and child host relationships.</span></a>
			<a class="action-card" href="<?php echo $cgi_base_url; ?>/avail.cgi"><strong>Availability</strong><span>Build host, service, and group availability reports.</span></a>
			<a class="action-card" href="<?php echo $cgi_base_url; ?>/extinfo.cgi?type=6"><strong>Scheduled downtime</strong><span>Review active and upcoming maintenance windows.</span></a>
			<a class="action-card" href="<?php echo $cgi_base_url; ?>/config.cgi"><strong>Configuration</strong><span>Inspect the objects loaded by the running core.</span></a>
		</div>
	</section>

	<footer id="mainfooter">
		<p id="maincopy">Copyright &copy; 2010-<?php echo $this_year; ?> Nagios Core Development Team and Community Contributors. Copyright &copy; 1999-2009 Ethan Galstad.</p>
		<p class="disclaimer">Nagios Core is provided under the GNU General Public License without warranty. Nagios marks are governed by the <a href="https://www.nagios.com/legal/trademarks/" target="_blank" rel="noopener noreferrer">trademark use restrictions</a>.</p>
	</footer>
</body>
</html>
