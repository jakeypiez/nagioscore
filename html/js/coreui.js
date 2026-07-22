(function () {
	'use strict';

	function parentTheme() {
		if (window.parent !== window) {
			try {
				var parentRoot = window.parent.document.documentElement;
				if (parentRoot.dataset.theme === 'light' || parentRoot.classList.contains('light')) {
					return 'light';
				}
				if (parentRoot.dataset.theme === 'dark' || parentRoot.classList.contains('dark')) {
					return 'dark';
				}
			} catch (error) {
				/* Direct CGI access and cross-origin frames use the saved theme. */
			}
		}
		try {
			var storedTheme = window.localStorage.getItem('nagios-core-colour-theme');
			if (storedTheme === 'light' || storedTheme === 'dark') {
				return storedTheme;
			}
		} catch (error) {
			/* Storage can be unavailable without affecting page rendering. */
		}
		return document.documentElement.classList.contains('light') ? 'light' : 'dark';
	}

	function applyTheme() {
		var theme = parentTheme();
		document.documentElement.classList.toggle('light', theme === 'light');
		document.documentElement.classList.toggle('dark', theme === 'dark');
		document.documentElement.dataset.theme = theme;
	}

	function enhanceLinks() {
		document.querySelectorAll('a[target="_blank"]').forEach(function (link) {
			var rel = new Set((link.getAttribute('rel') || '').split(/\s+/).filter(Boolean));
			rel.add('noopener');
			rel.add('noreferrer');
			link.setAttribute('rel', Array.from(rel).join(' '));
		});

		document.querySelectorAll('a[data-history-step]').forEach(function (link) {
			link.addEventListener('click', function (event) {
				event.preventDefault();
				window.history.go(Number(link.dataset.historyStep) || -1);
			});
		});

		document.querySelectorAll('a[data-context-help]').forEach(function (link) {
			link.addEventListener('click', function (event) {
				event.preventDefault();
				window.open(link.href, 'cshw', 'width=550,height=600,toolbar=0,location=0,status=0,resizable=1,scrollbars=1,noopener');
			});
		});
	}

	function enhanceForms() {
		var usedIds = new Set(Array.from(document.querySelectorAll('[id]')).map(function (element) {
			return element.id;
		}));
		document.querySelectorAll('input, select, textarea, button').forEach(function (control) {
			if (!control.id && control.name) {
				var baseId = 'nagios-' + control.name.replace(/[^a-zA-Z0-9_-]/g, '-');
				var candidateId = baseId;
				var suffix = 2;
				while (usedIds.has(candidateId)) {
					candidateId = baseId + '-' + suffix;
					suffix += 1;
				}
				control.id = candidateId;
				usedIds.add(candidateId);
			}
		});

		document.querySelectorAll('select[data-sync-host]').forEach(function (select) {
			function syncHost() {
				if (select.form && select.form.elements.host && typeof window.gethostname === 'function') {
					select.form.elements.host.value = window.gethostname(select.selectedIndex);
				}
			}
			select.addEventListener('focus', syncHost);
			select.addEventListener('change', syncHost);
		});
	}

	function initialiseLegacyStatusControls() {
		var topPages = document.getElementById('top_page_numbers');
		var bottomPages = document.getElementById('bottom_page_numbers');
		if (topPages && bottomPages && !topPages.dataset.synced) {
			Array.from(bottomPages.childNodes).forEach(function (node) {
				topPages.appendChild(node.cloneNode(true));
			});
			topPages.dataset.synced = 'true';
		}

		document.querySelectorAll('select[data-limit-url]').forEach(function (select) {
			select.addEventListener('change', function () {
				window.location.assign(select.dataset.limitUrl + '&limit=' + encodeURIComponent(select.value));
			});
		});
	}

	function initialisePageTour() {
		if (!document.body.dataset.pageTourUrl || typeof window.vidbox !== 'function') {
			return;
		}
		new window.vidbox({
			pos: 'lr',
			vidurl: document.body.dataset.pageTourUrl,
			text: '<a href="https://www.nagios.com/tours" target="_blank" rel="noopener noreferrer">Watch the complete Nagios Core tour</a>',
			vidid: document.body.dataset.pageTourId + ';' + (document.body.dataset.pageTourUser || '')
		});
	}

	function initialise() {
		applyTheme();
		enhanceLinks();
		enhanceForms();
		initialiseLegacyStatusControls();
		initialisePageTour();
		document.documentElement.classList.add('coreui-ready');
	}

	applyTheme();
	if (document.readyState === 'loading') {
		document.addEventListener('DOMContentLoaded', initialise, { once: true });
	} else {
		initialise();
	}
	window.addEventListener('storage', applyTheme);
}());
