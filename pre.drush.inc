<?php

/**
 * @file
 * Drush integration for the pre module.
 */

define('PRE_INI_FILE', '.pre-google');

/**
 * Implements hook_drush_command().
 */
function pre_drush_command() {
  $items['pre-update-ga'] = array(
    'description' => dt('Update GA visitor flow data'),
    'options' => array(
      'property' => dt('Set GA property ID'),
    ),
    'examples' => array(
      'drush pre-update-ga --property=UA-XXXXXXX-YY' => 'Update Google Analytics data for a specific property ID UA-XXXXXXX-YY (otherwise uses default property defined in <private>/.pre-google)',
    ),
  );
  return $items;
}

/**
 * Implements hook_cron().
 */
function pre_cron() {
  // Default to a daily interval
  $interval = variable_get('pre_interval', 86400);

  // We usually don't want to act every time cron runs (which could be every
  // minute) so keep a time for the next run in a variable.
  if (time() >= variable_get('pre_next_execution', 0)) {
    pre_update_ga();
    watchdog('pre', 'pre_cron executed');
    variable_set('pre_next_execution', time() + $interval);
  }
}

/**
 * Implements hook_drush_help().
 */
function pre_drush_help($section) {
  switch ($section) {
    case 'drush:pre-update-ga':
      return dt('Update GA visitor flow data');
  }
}

/**
 * Drush pre-update-ga command callback (inferred)
 */
function drush_pre_update_ga() {
  pre_update_ga();
}

function pre_log($status, $message) {
  drupal_set_message($message, $status);
}

/*
 * Load settings and then fetch and process GA data
 */
function pre_update_ga() {
  $option_property = drush_get_option('property');

  $settings = pre_get_settings();

  if ($settings === FALSE) {
    pre_log('error', dt('Error gettings settings from ini file @file.', array('@file' => PRE_INI_FILE)));
  }
  else {
    if (!array_key_exists('user', $settings) || !array_key_exists('pass', $settings)) {
      pre_log('error', dt('Missing user and/or pass in ini file.'));
    }
  }

  $property = $option_property;

  // default property is supplied by ini file
  if (empty($property) && array_key_exists('property', $settings)) {
    $property = $settings['property'];
  }

  // fetch and process ga data with user, pass, property
  pre_update_ga_process($settings['user'], $settings['pass'], $property);
}

/*
 * Read ini file from private file path
 * @return $settings (array) or FALSE on error
 */
function pre_get_settings() {
  $root = drush_get_context('DRUSH_DRUPAL_ROOT');
  $private_path = variable_get('file_private_path', FALSE);

  if ($private_path && file_exists($private_path)) {
    $ini_file = $root . DIRECTORY_SEPARATOR . $private_path . DIRECTORY_SEPARATOR . PRE_INI_FILE;
    $settings = parse_ini_file($ini_file);
    return $settings;
  }
  else {
    return FALSE;
  }
}

/*
 * Update GA data for specified GA property
 *
 * @param $user (string) Google account username
 * @param $pass (string) Google account password
 * @param $property (string) Google Analytics Property ID
 */
function pre_update_ga_process($user, $pass, $property) {
  $root = drush_get_context('DRUSH_DRUPAL_ROOT');
  $lib_path = $root . DIRECTORY_SEPARATOR . drupal_get_path('module', 'pre') . DIRECTORY_SEPARATOR . 'lib';
  $ga_script = $lib_path . DIRECTORY_SEPARATOR . 'report.rb';

  if (file_exists($ga_script)) {
    $descriptorspec = array(
      0 => array("pipe", "r"), // stdin is a pipe that the child will read from
      1 => array("pipe", "w"), // stdout is a pipe that the child will write to
      2 => array("file", "/tmp/drupal-pre-error.txt", "a") // stderr is a file to write to
    );

    $process = proc_open("ruby $ga_script $user $pass $property", $descriptorspec, $pipes);

    if (is_resource($process)) {
      // $pipes now looks like this:
      // 0 => writeable handle connected to child stdin
      // 1 => readable handle connected to child stdout
      // Any error output will be appended to /tmp/error-output.txt

      // @todo stdin?
      // fwrite($pipes[0], $input);
      // fclose($pipes[0]);

      $stdout = stream_get_contents($pipes[1]);
      fclose($pipes[1]);

      // It is important that you close any pipes before calling
      // proc_close in order to avoid a deadlock
      $return_value = proc_close($process);
      if ($return_value == 0) {
        $result = json_decode($stdout);
        // $result is a nested array with values
        // [source path, destination path, probability]
        foreach($result as $row) {
          list($source, $destination, $probability) = $row;
          // @todo replace values into database (key = [source, destination])
        }
        pre_log('status', t('Successfully processed GA data for property @property', array('@property' => $property)));
      }
      else {
        pre_log('error', t('Error processing GA data for property @property', array('@property' => $property)));
      }
    }
  }
  else {
    pre_log('error', t('GA script @script not found', array('@script' => $ga_script)));
  }
}
