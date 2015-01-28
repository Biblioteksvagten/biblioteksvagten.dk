<?php

/**
 * @file
 * Drush commands for the Vopros chat module.
 */

/**
 * Implements hook_drush_command().
 */
function vopros_chat_drush_command() {
  $items = array();

  $items['vopros-chat-start'] = array(
    'description' => 'Start Vopros chat server.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL,
    'options' => array(
      'node-debug' => 'Show debugging output.',
      'nodemon' => 'Run with nodemon.',
      'daemon' => 'Spawn daemon process.',
    ),
  );

  $items['vopros-chat-stop'] = array(
    'description' => 'Stop Vopros chat server, when started with --daemon.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL,
  );

  $items['vopros-chat-package-json'] = array(
    'description' => 'Generate Vopros chat package.json.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL,
  );

  return $items;
}

/**
 * Command callback for starting server.
 */
function drush_vopros_chat_start() {
  $method = 'node';
  if (drush_get_option('nodemon', FALSE)) {
    if (drush_get_option('daemon', FALSE)) {
      return drush_set_error("--nodemon and --daemon doesn't make sense together.");
    }
    $method = 'nodemon';
  }
  if (drush_get_option('daemon', FALSE)) {
    $method = 'daemon';
  }

  _vopros_chat_start_stop_server(TRUE, $method, 'drush_shell_exec_interactive', drush_get_option('node-debug', FALSE));
}

/**
 * Command callback for stopping server.
 */
function drush_vopros_chat_stop() {
  _vopros_chat_start_stop_server(FALSE, 'daemon', 'drush_shell_exec_interactive');
}

/**
 * Command callback for generating package.json.
 */
function drush_vopros_chat_package_json() {
  $nodejs_path = getcwd() . '/' . drupal_get_path('module', 'nodejs');
  $vopros_chat_path = getcwd() . '/' . drupal_get_path('module', 'vopros_chat');
  $info = drupal_parse_info_file($vopros_chat_path . '/vopros_chat.info');

  // Read package.json from nodejs.module.
  $package_json = json_decode(file_get_contents($nodejs_path . '/package.json'));

  // Fix some entries.
  $package_json->name = 'Vopros_Chat';
  $package_json->homepage = 'https://github.com/reload/vopros_chat';
  $package_json->description = $info['description'];
  if (!empty($info['version'])) {
    $package_json->version = preg_replace('/x-/', '', $info['version']);
  }
  $package_json->dependencies->drupal = '0.5.2';
  $package_json->dependencies->hashish = '0.0.4';
  $package_json->dependencies->{'sync-exec'} = '0.3.2';
  $package_json->dependencies->daemon = '1.1.0';

  // Write it as our own package.json.
  file_put_contents($vopros_chat_path . '/server/package.json', drupal_json_encode($package_json));
}