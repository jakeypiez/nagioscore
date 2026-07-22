(function () {
	'use strict';

	var body = document.body;
	var apiBase = body.dataset.apiBase || '';
	var refreshTimer = null;
	var refreshInterval = 60000;
	var maximumStatusAge = Math.max(120, Number(body.dataset.statusMaxAge) || 120);

	function setText(selector, value) {
		var element = typeof selector === 'string' ? document.querySelector(selector) : selector;
		if (element) {
			element.textContent = String(value);
		}
	}

	function statusAgeInSeconds(queryTime, lastUpdate) {
		if (!Number.isFinite(queryTime) || !Number.isFinite(lastUpdate) || lastUpdate <= 0) {
			return null;
		}
		// Current statusjson.cgi timestamps are milliseconds since the epoch.
		// Older/custom endpoints may still return seconds, so support both forms.
		var unitsPerSecond = Math.max(Math.abs(queryTime), Math.abs(lastUpdate)) > 100000000000 ? 1000 : 1;
		return Math.max(0, (queryTime - lastUpdate) / unitsPerSecond);
	}

	function fetchStatus(query) {
		return fetch(apiBase + '/statusjson.cgi?query=' + encodeURIComponent(query), {
			credentials: 'same-origin',
			headers: { Accept: 'application/json' },
			cache: 'no-store'
		}).then(function (response) {
			if (!response.ok) {
				var error = new Error(response.status === 401 ? 'Authentication required' : 'Status API unavailable');
				error.status = response.status;
				throw error;
			}
			return response.json().then(function (payload) {
				if (!payload || !payload.result || Number(payload.result.type_code) !== 0 || !payload.data) {
					var message = payload && payload.result && payload.result.message;
					throw new Error(message || 'Status API returned an invalid response');
				}
				var queryTime = Number(payload.result.query_time);
				var lastUpdate = Number(payload.result.last_data_update);
				var age = statusAgeInSeconds(queryTime, lastUpdate);
				if (age === null || age > maximumStatusAge) {
					throw new Error(age === null ? 'Monitoring data freshness is unknown' : 'Monitoring data is stale (' + Math.floor(age) + ' seconds old)');
				}
				return payload;
			});
		});
	}

	function updateEngine(payload) {
		var status = payload && payload.data && payload.data.programstatus;
		var statusElement = document.getElementById('core-status');
		var dot = statusElement ? statusElement.querySelector('.status-dot') : null;
		if (!status || !status.nagios_pid) {
			throw new Error('Core process is not reporting a PID');
		}

		setText(statusElement, (status.daemon_mode ? 'Daemon' : 'Process') + ' running');
		if (statusElement) {
			dot = document.createElement('span');
			dot.className = 'status-dot is-running';
			dot.setAttribute('aria-hidden', 'true');
			statusElement.prepend(dot);
		}
		setText('#core-detail', 'PID ' + status.nagios_pid + ' · status API connected');
	}

	function updateEngineError(error) {
		var statusElement = document.getElementById('core-status');
		setText(statusElement, error && error.status === 401 ? 'Authentication required' : 'Status unavailable');
		if (statusElement) {
			var dot = document.createElement('span');
			dot.className = 'status-dot is-error';
			dot.setAttribute('aria-hidden', 'true');
			statusElement.prepend(dot);
		}
		setText('#core-detail', error && error.message ? error.message : 'Unable to read program status');
	}

	function updateCounts(kind, payload) {
		var count = payload && payload.data && payload.data.count;
		if (!count || typeof count !== 'object') {
			throw new Error('Status totals are unavailable');
		}
		var keys = kind === 'host' ? ['up', 'down', 'unreachable', 'pending'] : ['ok', 'warning', 'critical', 'unknown', 'pending'];
		var total = keys.reduce(function (sum, key) { return sum + (Number(count[key]) || 0); }, 0);
		setText('#' + kind + '-total', total);
		keys.forEach(function (key) {
			setText('[' + (kind === 'host' ? 'data-host-state' : 'data-service-state') + '="' + key + '"]', Number(count[key]) || 0);
		});
	}

	function updateCountsError(kind) {
		var attribute = kind === 'host' ? 'data-host-state' : 'data-service-state';
		setText('#' + kind + '-total', '—');
		document.querySelectorAll('[' + attribute + ']').forEach(function (element) {
			setText(element, '—');
		});
	}

	function applyResult(result, onSuccess, onError) {
		if (result.status !== 'fulfilled') {
			onError(result.reason);
			return;
		}
		try {
			onSuccess(result.value);
		} catch (error) {
			onError(error);
		}
	}

	function refresh() {
		Promise.allSettled([
			fetchStatus('programstatus'),
			fetchStatus('hostcount'),
			fetchStatus('servicecount')
		]).then(function (results) {
			applyResult(results[0], updateEngine, updateEngineError);
			applyResult(results[1], function (payload) { updateCounts('host', payload); }, function () { updateCountsError('host'); });
			applyResult(results[2], function (payload) { updateCounts('service', payload); }, function () { updateCountsError('service'); });
		});
	}

	function stopRefreshing() {
		if (refreshTimer !== null) {
			window.clearInterval(refreshTimer);
			refreshTimer = null;
		}
	}

	function startRefreshing() {
		if (document.visibilityState === 'hidden') {
			return;
		}
		stopRefreshing();
		refresh();
		refreshTimer = window.setInterval(refresh, refreshInterval);
	}

	function initialiseTour() {
		if (body.dataset.tourEnabled !== '1' || typeof window.vidbox !== 'function') {
			return;
		}
		new window.vidbox({
			pos: 'lr',
			vidurl: 'https://www.youtube-nocookie.com/embed/2hVBAet-XpY',
			text: '<a href="https://www.nagios.com/tours" target="_blank" rel="noopener noreferrer">Watch the complete Nagios Core tour</a>',
			vidid: 'main;' + (body.dataset.tourUser || '')
		});
	}

	function initialise() {
		initialiseTour();
		startRefreshing();
	}

	document.addEventListener('visibilitychange', function () {
		if (document.visibilityState === 'hidden') {
			stopRefreshing();
		} else {
			startRefreshing();
		}
	});
	window.addEventListener('pagehide', stopRefreshing);

	if (document.readyState === 'loading') {
		document.addEventListener('DOMContentLoaded', initialise, { once: true });
	} else {
		initialise();
	}
}());
