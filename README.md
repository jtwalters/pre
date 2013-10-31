# pre
A prerender/prefetch/dns-prefetch helper with visitor flow knowledge provided by Google Analytics.

## Requirements
You must have the `google_analytics_api` (7.x-3.x) module which is provided by the `google_analytics_reports` module.

Next, you need to visit `admin/config/system/google-analytics-reports` to authenticate with Google (via oauth).

## Usage
* Check the status report page to verify the module has up to date GA visitor flow data.
* Configure the module settings at `admin/config/system/pre`.

## Future Features
* prefetch helper
* dns-prefetch helper (also see the CDN module)