<?php

/**
 * @file
 * Drush integration for the Commerce payeezy module.
 */

/**
 * Implements hook_drush_command().
 */
function commerce_payeezy_drush_command() {
  $items['payeezy-download'] = array(
    'callback' => 'drush_commerce_payeezy_payeezy_download',
    'description' => dt('Downloads the payeezy PHP library from https://github.com/payeezy/payeezy-php.'),
  );
  return $items;
}

/**
 * A command callback.
 */
function drush_commerce_payeezy_payeezy_download() {

// Get the path from the argument, if site, or use the default.
  $drush_context = drush_get_context('DRUSH_DRUPAL_ROOT');

  // Can we use Libraries API?
  if (module_exists('libraries') && function_exists('libraries_get_path')) {
    $libraries_dir = libraries_get_path('payeezy-php');
  }

  // Load the path.
  $path = $libraries_dir ? $drush_context . '/' . $libraries_dir : $drush_context . '/sites/all/libraries/payeezy-php';

  drush_log(dt('Download destination path: "@path"', array('@path' => $path)), 'notice');

  // Set the directory to the download location.
  $olddir = getcwd();

  $download_path = str_replace('/payeezy-php', '', $path);

  chdir($download_path);

  $download_uri = 'https://github.com/ndubbaka/payeezy-php/releases/download/v1.0-alpha1/payeezy-php-v1.0-alpha1.zip';

  // Download the zip archive.
  if ($filepath = drush_download_file($download_uri, $download_path . '/payeezy-php-v1.0-alpha1.zip')) {
    $filename = basename($filepath);
    // Decompress the zip archive.
    drush_tarball_extract($filename);
    $dirname = 'payeezy-php';
  }

  if (is_dir($dirname)) {
    drush_log(dt('payeezy-php library has been installed in @path', array('@path' => $path)), 'success');
  }
  else {
    drush_log(dt('Drush was unable to install the payeezy-php library to @path', array('@path' => $path)), 'error');
  }

  // Set working directory back to the previous working directory.
  chdir($olddir);
}
