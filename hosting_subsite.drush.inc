<?php
/**
 * Drush command interface.
 *
 * Injects necessary configuration for setting up subsite.
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
function hosting_subsite_provision_apache_vhost_config($url, $options) {
  // List subsites
  drush_log(dt('Listing subsites...'));
  $result = db_query("SELECT n2.title name, l.path, p.publish_path platform
      FROM {node} n1
        JOIN {hosting_subsite} l ON n1.type = 'site' AND n1.nid = l.parent
        JOIN {node} n2 ON n2.type = 'site' AND l.subsite = n2.nid
        JOIN {hosting_site} s ON n2.nid = s.nid
        JOIN {hosting_platform} p ON s.platform = p.nid
      WHERE n1.title = '%s' AND s.status = %d AND p.status = %d
      ORDER BY l.path", $url, HOSTING_SITE_ENABLED, HOSTING_PLATFORM_ENABLED);

  $subsites = array();
  while ($subsite = db_fetch_object($result)) {
    $subsites[] = $subsite;
  }
  if (count($subsites) == 0) {
    drush_log(dt('No subsites found.', 'success'));
    return NULL;
  } else {
    drush_log(dt('Found @subsites subsites.', array('@subsites' => count($subsites))), 'success');
  }

  // Load subsite details
  $valid_count = 0;
  foreach ($subsites as &$subsite) {
    // Load drushrc.php and extract database init config
    $drushrc = rtrim($subsite->platform, '/') . '/sites/' . $subsite->name . '/drushrc.php';
    drush_log(dt('Loading subsite @site (@drushrc)...', array('@site' => $subsite->name, '@drushrc' => $drushrc)));
    // Check file exists
    if (!provision_path('exists', $drushrc)) {
      drush_log(dt('Configuration not found (@drushrc). Ignoring subsite.', array('@drushrc' => $drushrc)), 'warning');
      $subsite->valid = FALSE;
      continue;
    }
    // Check file readable
    elseif (!provision_path('readable', $drushrc)) {
      drush_log(dt('Configuration not readable (@drushrc). Ignoring subsite.', array('@drushrc' => $drushrc)), 'warning');
      $subsite->valid = FALSE;
      continue;
    }
    // Load contents
    $options = _hosting_subsite_load_options($drushrc);
    $subsite->db_type   = $options['db_type'];
    $subsite->db_name   = $options['db_name'];
    $subsite->db_user   = $options['db_user'];
    $subsite->db_passwd = $options['db_passwd'];
    $subsite->db_host   = $options['db_host'];
    $subsite->valid = TRUE;
    $valid_count ++;
    drush_log(dt('Subsite @site loaded.', array('@site' => $subsite->name)));
  }

  // Check number of valid subsites
  if ($valid_count == 0) {
    drush_log(dt('Could not load any subsite.'), 'warning');
    return NULL;
  }

  // Prepare platforms
  $platforms = array();
  foreach ($subsites as $subsite) {
    if (!$subsite->valid) {
      continue;
    }
    $platforms[$subsite->platform] = $subsite->platform;
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
      AllowOverride FileInfo
    </Directory>

EOT;
  }
  drush_log(dt('Generated platforms configuration.'), 'success');

  // Configure subsites
  foreach ($subsites as $subsite) {
    if (!$subsite->valid) {
      continue;
    }
    $platform_clean = _hosting_subsite_escape_publish_path($subsite->platform);
    $path_clean = '/' . drupal_urlencode($subsite->path);
    $config .= <<<EOT

    Alias "$path_clean" "$platform_clean"
    <Location "$path_clean">
      SetEnv db_type    {$subsite->db_type}
      SetEnv db_name    {$subsite->db_name}
      SetEnv db_user    {$subsite->db_user}
      SetEnv db_passwd  {$subsite->db_passwd}
      SetEnv db_host    {$subsite->db_host}
    </Location>

EOT;
  }
  drush_log(dt('Generated subsites configuration.'), 'success');
  return "    # Subsite configuration\n" . $config . "\n";
}

/**
 * Remember to remove the subsite links.
 */
function drush_hosting_subsite_post_provision_delete() {
  // @TODO
}

/**
 * Re-verify parent site after disabling.
 */
function drush_hosting_subsite_post_provision_disable() {
  // @TODO
}

/**
 * Re-verify parent site after enabling.
 */
function drush_hosting_subsite_post_provision_enable() {
  // @TODO
}

/**
 * Re-verify parent site after migrating.
 * @TODO Provision migrate vs. verify: will 'verify' flood parent 'verify'?
 */
function drush_hosting_subsite_post_provision_migrate() {
  // @TODO
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
