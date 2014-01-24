<?php

/*
 * @file
 * Helper functions for the pre module
 */


/*
 * Get path to current page
 */
function pre_current_path() {
  return request_uri();
}

/**
 * Get all path data for reporting purposes
 */
function pre_get_path_data() {
    $result = db_select('pre_visitor_flow', 'p')
    ->fields('p', array('source_path', 'destination_path', 'probability'))
    ->extend('PagerDefault')
    ->orderBy('probability', 'DESC')
    ->limit(PRE_REPORT_ITEMS_PER_PAGE)
    ->execute();
  if ($result->rowCount() > 0) {
    $rows = array();
    while ($row = $result->fetchAssoc()) {
      $rows[] = $row;
    }
    return $rows;
  }
  else {
    return FALSE;
  }
}

/*
 * Get next page data for a given path
 */
function pre_next_probability($path, $limit = PRE_MAX_DESTINATIONS) {
  $result = db_select('pre_visitor_flow', 'p')
    ->fields('p', array('source_path', 'destination_path', 'probability'))
    ->condition('source_path', $path, 'LIKE')
    ->orderBy('probability', 'DESC')
    ->range(0, $limit)
    ->execute();
  if ($result->rowCount() > 0) {
    $rows = array();
    while ($row = $result->fetchAssoc()) {
      $rows[] = array(
        'destination_path' => $row['destination_path'],
        'probability' => $row['probability'],
      );
    }
    return $rows;
  }
  else {
    return FALSE;
  }
}

/*
 * Fetch and save GA visitor flow data to local database
 */
function pre_update_ga() {
  $profile = variable_get('google_analytics_reports_profile_id', 0);
  $data = pre_get_visitor_flow();

  if (is_array($data)) {
    foreach ($data as $row) {
      // insert/update probability value for key [source, destination]
      db_merge('pre_visitor_flow')
        ->key(array(
          'source_path' => $row['source'],
          'destination_path' => $row['destination'],
        ))
        ->fields(array(
          'probability' => $row['probability'],
        ))
        ->execute();
    }

    pre_log('status', t('Successfully processed GA data for profile @profile.', array('@profile' => $profile)));
  }
  else {
    pre_log('error', t('Error. Could not process GA data for profile @profile.', array('@profile' => $profile)));
    return FALSE;
  }
  return TRUE;
}

/**
 * Retrieves visitor flow data
 */
function pre_get_visitor_flow() {
  $data = array();
  $params = array(
    'metrics' => array('ga:pageviews'),
    'dimensions' => array('ga:pagePath'),
    'sort_metric' => array('-ga:pageviews'),
    'start_date' => strtotime('-31 days'),
    'end_date' => strtotime('-1 day'),
    'sort' => '-ga:pageviews',
    'max_results' => PRE_MAX_SOURCES,
  );
  $source_feed = google_analytics_api_report_data($params);
  if ($source_feed->error) {
    return FALSE;
  }
  foreach ($source_feed->results->rows as $source_row) {
    $params = array(
      'metrics' => array('ga:pageviews'),
      'dimensions' => array('ga:pagePath'),
      'sort_metric' => array('-ga:pageviews'),
      'start_date' => strtotime('-31 days'),
      'end_date' => strtotime('-1 day'),
      'sort' => '-ga:pageviews',
      'max_results' => PRE_MAX_DESTINATIONS,
      'filters' => _pre_path_filter($source_row['pagePath'], 'ga:previousPagePath'),
    );
    $dest_feed = google_analytics_api_report_data($params);
    if ($dest_feed->error) {
      return FALSE;
    }
    foreach ($dest_feed->results->rows as $dest_row) {
      // skip if paths match
      if ($source_row['pagePath'] == $dest_row['pagePath']) {
        continue;
      }
      $probability = $dest_row['pageviews'] / $source_row['pageviews'];
      $data[] = array(
        'source' => $source_row['pagePath'],
        'destination' => $dest_row['pagePath'],
        'probability' => $probability,
      );
    }
  }
  return $data;
}

/*
 * Construct a filter string that grabs pagePaths corresponding to the specified path.
 */
function _pre_path_filter($path, $type = 'ga:pagePath') {
  // Google Analytics regex filters have a limit of 32 characters. Therefore we
  // can't simply create one filter per pagePath. Instead we are going too do a
  // "contains substring" match on the path, and then take as many of the ending
  // characters paired with ([?#].*)?$.

  // Use 100 character maximum to allow some room for escaping regex metacharacters.
  if (strlen($path) <= 121) {
    $filter = "$type=~^" . preg_quote($path) . '(#.*)?$';
  }
  else {
    $filter = "$type=@$path;$type=~" . preg_quote(substr($path, -100)) . '(#.*)?$';
  }
  return $filter;
}

/*
 * Logging helper
 */
function pre_log($status, $message) {
  drupal_set_message(check_plain($message), $status);
}
