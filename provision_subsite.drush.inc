<?php
/**
 * Subsite provisioning plugin.
 *
 * Injects necessary configuration for setting up subsites.
 *
 * @author Taihao Zhang (zhang.taihao@gmail.com)
 * @license GPLv2. See LICENSE.
 */

/*
Approximate format of injected code:

    Alias /%subsite:path% "%platform:publish_path%"
    <Location /%subsite:path%>
      SetEnv db_type    xxxxx
      SetEnv db_name    xxxxx
      SetEnv db_user    xxxxx
      SetEnv db_passwd  xxxxx
      SetEnv db_host    xxxxx
    </Location>

    <Directory "%platform:publish_path%">
      AllowOverride FileInfo
    </Directory>

The central idea is to set up alias and db parameters per subsite, and
re-enable rewrite rules per platform.

See:
  drupal_urlencode()

To escape directory paths (yes, even for Apache!):
 1. Add slashes to double quotes
 2. Enclose string in double quotes to account for spaces
*/

/**
 * Inject apache subsite config.
 */
function provision_subsite_provision_apache_vhost_config($url, $options) {
  if (!$options['hosting_subsite']) {
    drush_log(dt('To generate subsite configuration, provision-verify must be run through hosting-task.'), 'warning');
  }

  $publish_path = $options['publish_path'];

  $subsites = $options['subsites'];
  if (empty($subsites)) {
    drush_log(dt('No subsites to generate configuration for.'));
    return NULL;
  }

  drush_log(dt('Generating subsites configuration...'));

  // Prepare platforms
  $platforms = array();
  foreach ($subsites as &$subsite) {
    if (!$subsite['valid']) {
      continue;
    }
    $platforms[$subsite['platform']] = $subsite['platform'];
  }
  $platforms = array_keys($platforms);
  drush_log(dt('Detected @platforms platforms.', array('@platforms' => count($platforms))));

  // Start building config
  $config = '';

  // Configure platforms
  foreach ($platforms as $platform) {
    $platform_clean = _hosting_subsite_escape_publish_path($platform);
    $config .= <<<EOT

    <Directory "$platform_clean">
      AllowOverride All
    </Directory>

EOT;
  }
  drush_log(dt('Generated platforms configuration.'), 'success');

  // Configure subsites
  foreach ($subsites as $subsite) {
    if (!$subsite['valid']) {
      continue;
    }

    // @TODO Crude mechanism to set up symlink for now
    $sym_path = $publish_path . '/' . $subsite['path'];
    $sym_target = $subsite['platform'];
    $failure = FALSE;
    if (file_exists($sym_path)) {
      if (!is_link($sym_path) || is_file($sym_path)) {
        $failure = TRUE;
      }
    }
    if (!$failure) {
      // Try to recreate symbolic link
      if (@unlink($sym_path)) {
        $failure = !symlink($sym_target, $sym_path);
      } else {
        $failure = TRUE;
      }
    }
    if ($failure) {
      drush_log(dt('Cannot set up @subsite at @path. Skipped.', array(
        '@subsite' => $subsite['name'], '@path' => '/' . $subsite['path'],
      )), 'warning');
      continue;
    }

    //$platform_clean = _hosting_subsite_escape_publish_path($subsite['platform']);
    $platform_clean = _hosting_subsite_escape_publish_path($sym_path);
    $path_clean = '/' . drupal_urlencode($subsite['path']);
    $config .= <<<EOT

    Alias "$path_clean" "$platform_clean"
    <Directory "$platform_clean">
      SetEnv db_type    {$subsite[db_type]}
      SetEnv db_name    {$subsite[db_name]}
      SetEnv db_user    {$subsite[db_user]}
      SetEnv db_passwd  {$subsite[db_passwd]}
      SetEnv db_host    {$subsite[db_host]}
    </Directory>

EOT;
  }
  drush_log(dt('Generated subsites configuration.'), 'success');
  return "    # Subsite configuration\n" . $config . "\n";
}

/**
 * Escape platform publish path.
 */
function _hosting_subsite_escape_publish_path($path) {
  static $_cache = array();
  if (!isset($_cache[$path])) {
    $_cache[$path] = str_replace(array("\\", "\""), array("\\\\", "\\\""), $path);
  }
  return $_cache[$path];
}

/**
 * Load drushrc.php config $options array.
 */
function _hosting_subsite_load_options($drushrc) {
  static $_options = array();
  if (!isset($_options[$drushrc])) {
    @include_once($drushrc);
    $_options[$drushrc] = $options;
  }
  return $_options[$drushrc];
}
