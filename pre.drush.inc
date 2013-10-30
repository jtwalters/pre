<?php

/**
 * @file
 * Drush integration for the pre module.
 */

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
