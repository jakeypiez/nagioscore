(function () {
	'use strict';

	var root = document.documentElement;
	var body = document.body;
	var navToggle = document.getElementById('nav-toggle');
	var navScrim = document.getElementById('navigation-scrim');
	var navigationPanel = document.getElementById('navigation-panel');
	var contentPanel = document.getElementById('main-content');
	var navFrame = document.getElementById('navigation-frame');
	var contentFrame = document.getElementById('content-frame');
	var themeToggle = document.getElementById('theme-toggle');
	var themeLabel = themeToggle ? themeToggle.querySelector('.theme-label') : null;
	var storageKey = 'nagios-core-colour-theme';
	var mobileNavigation = window.matchMedia('(max-width: 760px)');

	function storedTheme() {
		try {
			var value = window.localStorage.getItem(storageKey);
			return value === 'light' || value === 'dark' ? value : null;
		} catch (error) {
			return null;
		}
	}

	function applyTheme(theme, persist) {
		var nextTheme = theme === 'light' ? 'light' : 'dark';
		root.classList.toggle('light', nextTheme === 'light');
		root.classList.toggle('dark', nextTheme === 'dark');
		root.dataset.theme = nextTheme;

		if (themeToggle) {
			themeToggle.setAttribute('aria-label', nextTheme === 'dark' ? 'Use light theme' : 'Use dark theme');
			themeToggle.setAttribute('title', nextTheme === 'dark' ? 'Use light theme' : 'Use dark theme');
		}
		if (themeLabel) {
			themeLabel.textContent = nextTheme === 'dark' ? 'Light' : 'Dark';
		}

		if (persist) {
			try {
				window.localStorage.setItem(storageKey, nextTheme);
			} catch (error) {
				/* Storage can be disabled without affecting the monitoring UI. */
			}
		}

		[navFrame, contentFrame].forEach(function (frame) {
			try {
					if (frame && frame.contentDocument) {
						var frameRoot = frame.contentDocument.documentElement;
						frameRoot.classList.toggle('light', nextTheme === 'light');
						frameRoot.classList.toggle('dark', nextTheme === 'dark');
						frameRoot.dataset.theme = nextTheme;
				}
			} catch (error) {
				/* An explicitly external frame is never inspected. */
			}
		});
	}

	function setNavigation(open) {
		body.classList.toggle('nav-open', open);
		var hidden = mobileNavigation.matches && !open;
		var contentHidden = mobileNavigation.matches && open;
		if (navigationPanel) {
			navigationPanel.setAttribute('aria-hidden', hidden ? 'true' : 'false');
			if ('inert' in navigationPanel) {
				navigationPanel.inert = hidden;
			}
		}
		if (navFrame) {
			if (hidden) {
				navFrame.setAttribute('tabindex', '-1');
			} else {
				navFrame.removeAttribute('tabindex');
			}
		}
		if (contentPanel) {
			contentPanel.setAttribute('aria-hidden', contentHidden ? 'true' : 'false');
			if ('inert' in contentPanel) {
				contentPanel.inert = contentHidden;
			}
		}
		if (navToggle) {
			navToggle.setAttribute('aria-expanded', open ? 'true' : 'false');
			var label = navToggle.querySelector('.visually-hidden');
			if (label) {
				label.textContent = open ? 'Close navigation' : 'Open navigation';
			}
		}
	}

	function syncActiveNavigation() {
		try {
			var navDocument = navFrame.contentDocument;
			var current = new URL(contentFrame.contentWindow.location.href);
			var links = navDocument.querySelectorAll('a[target="main"]');
			var best = null;
			var bestScore = -1;

			links.forEach(function (link) {
				link.removeAttribute('aria-current');
				var candidate = new URL(link.href, current.href);
				if (candidate.pathname !== current.pathname) {
					return;
					}
					var score = 1;
					var matches = true;
					candidate.searchParams.forEach(function (value, key) {
						if (current.searchParams.get(key) === value) {
							score += 1;
						} else {
							matches = false;
						}
					});
					if (matches && score > bestScore) {
					best = link;
					bestScore = score;
				}
			});

			if (best) {
				best.setAttribute('aria-current', 'page');
			}
			} catch (error) {
				/* A frame can be briefly unavailable while its new page loads. */
			}
	}

	applyTheme(storedTheme() || root.dataset.configuredTheme || 'dark', false);
	setNavigation(false);
	var handleNavigationBreakpoint = function () { setNavigation(false); };
	if (typeof mobileNavigation.addEventListener === 'function') {
		mobileNavigation.addEventListener('change', handleNavigationBreakpoint);
	} else if (typeof mobileNavigation.addListener === 'function') {
		mobileNavigation.addListener(handleNavigationBreakpoint);
	}

	if (themeToggle) {
		themeToggle.addEventListener('click', function () {
			applyTheme(root.dataset.theme === 'dark' ? 'light' : 'dark', true);
		});
	}
	if (navToggle) {
		navToggle.addEventListener('click', function () {
			var opening = !body.classList.contains('nav-open');
			setNavigation(opening);
			if (opening && navFrame) {
				navFrame.focus();
			}
		});
	}
	if (navScrim) {
		navScrim.addEventListener('click', function () { setNavigation(false); });
	}
	if (contentFrame) {
		contentFrame.addEventListener('load', function () {
			applyTheme(root.dataset.theme, false);
			syncActiveNavigation();
			if (window.matchMedia('(max-width: 760px)').matches) {
				setNavigation(false);
			}
		});
	}
	if (navFrame) {
		navFrame.addEventListener('load', function () {
			applyTheme(root.dataset.theme, false);
			syncActiveNavigation();
		});
	}

	document.addEventListener('keydown', function (event) {
		if (event.key === 'Escape' && body.classList.contains('nav-open')) {
			setNavigation(false);
			navToggle.focus();
		}
	});

	window.addEventListener('storage', function (event) {
		if (event.key === storageKey && (event.newValue === 'light' || event.newValue === 'dark')) {
			applyTheme(event.newValue, false);
		}
	});
}());
