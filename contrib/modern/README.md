# Nagios Core modern presentation layer

This directory contains an additive presentation layer for the existing Nagios Core PHP shell and CGI-generated pages. It deliberately does not replace CGI routes, query parameters, forms, authentication, authorization, refresh headers, JSON endpoints, image responses, CSV output, WML output, or embedded output.

## Integration

Install stylesheets/theme.css into the public stylesheets directory and load it after common.css and the page-specific stylesheet. Loading it last is important because the existing Exfoliation and Classic stylesheets define many of the same legacy selectors.

The stylesheet has no external network dependencies. It uses the operating system font stack and existing markup, so it remains usable when the monitoring interface is isolated from the internet.

The default palette is dark. Light mode can be selected by adding class="light" or data-theme="light" to the document root. Dark mode can be made explicit with class="dark" or data-theme="dark". The selectors support the current PHP convention as well as a future shared CGI theme hook.

## Compatibility contract

The override intentionally targets the class and ID names emitted by Nagios Core 4.5.x, including:

- status, host, service, totals, pagination, and problem-state selectors;
- command, option, report, date, filter, notification, and log tables;
- Tactical Overview health, feature, host, service, outage, and performance blocks;
- Extended Information state, command, comment, downtime, queue, and performance panels;
- status map and trend popups without resizing their generated images or image-map coordinates;
- the PHP shell, sidebar, landing page, context help, page-tour widget, and JSON query generator.

Do not remove or rename hidden command fields such as cmd_typ, cmd_mod, or nagFormId when changing markup around this stylesheet. Those fields and the CGI-set NagFormId cookie are part of the command security flow.

The legacy pages contain deeply nested tables and several inline dimensions. On narrow screens the theme prioritizes readable controls and safe horizontal scrolling rather than converting tables into a different DOM structure. This avoids changing link targets, table relationships, image maps, or form submission behavior.

## Accessibility and resilience

The theme provides visible keyboard focus, large interactive targets, non-color state borders and labels where markup exposes state names, responsive spacing, high-contrast forced-colors support, reduced-motion handling, print styles, and color tokens designed for readable state contrast in both palettes.

Visual verification should cover at least the landing page, sidebar, Tactical Overview, host and service status, command confirmation, comments, downtime, scheduling queue, availability, trends, histogram, notifications, event log, configuration, context help, JSON query generator, and both HTML and generated-image map views.
