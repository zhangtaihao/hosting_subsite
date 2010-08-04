<?php
/**
 * Drush command interface.
 *
 * Loads subsites to be used in provision backend to inject Apache config.
 *
 * @author Taihao Zhang (zhang.taihao@gmail.com)
 * @license GPLv2. See LICENSE.
 */

/**
 * Load apache subsite config.
 */
function drush_hosting_subsite_pre_hosting_task($task) {
  $task =& drush_get_context('HOSTING_TASK');
  if ($task->ref->type == 'site' && $task->task_type == 'verify') {
    hosting_subsite_pre_hosting_site_verify_task($task);
  }
}

/**
 * Helper function as pre site verify hook.
 */
function hosting_subsite_pre_hosting_site_verify_task(&$task) {
  $url = $task->ref->title;

  $task->options['hosting_subsite'] = TRUE;

  // List subsites
  drush_log(dt('Preparing subsites...'));
  $result = db_query("SELECT n2.title name, l.path, p.publish_path platform
      FROM {node} n1
        JOIN {hosting_subsite} l ON n1.type = 'site' AND n1.nid = l.parent
        JOIN {node} n2 ON n2.type = 'site' AND l.subsite = n2.nid
        JOIN {hosting_site} s ON n2.nid = s.nid
        JOIN {hosting_platform} p ON s.platform = p.nid
      WHERE n1.title = '%s' AND s.status = %d AND p.status = %d",
      $url, HOSTING_SITE_ENABLED, HOSTING_PLATFORM_ENABLED);

  $subsites = array();
  while ($subsite = db_fetch_array($result)) {
    $subsites[] = $subsite;
  }
  if (count($subsites) == 0) {
    drush_log(dt('No subsites found.'));
    return;
  } else {
    drush_log(dt('Found @subsites subsites.', array('@subsites' => count($subsites))), 'success');
  }

  // Load subsite details
  $valid_count = 0;
  foreach ($subsites as &$subsite) {
    // Load drushrc.php and extract database init config
    $drushrc = rtrim($subsite['platform'], '/') . '/sites/' . $subsite['name'] . '/drushrc.php';
    drush_log(dt('Loading subsite @site (@drushrc)...', array('@site' => $subsite['name'], '@drushrc' => $drushrc)));
    // Check file exists
    if (!provision_path('exists', $drushrc)) {
      drush_log(dt('Configuration not found (@drushrc). Ignoring subsite.', array('@drushrc' => $drushrc)), 'warning');
      $subsite['valid'] = FALSE;
      continue;
    }
    // Check file readable
    elseif (!provision_path('readable', $drushrc)) {
      drush_log(dt('Configuration not readable (@drushrc). Ignoring subsite.', array('@drushrc' => $drushrc)), 'warning');
      $subsite['valid'] = FALSE;
      continue;
    }
    // Load contents
    $options = _hosting_subsite_load_options($drushrc);
    $subsite['db_type']   = $options['db_type'];
    $subsite['db_name']   = $options['db_name'];
    $subsite['db_user']   = $options['db_user'];
    $subsite['db_passwd'] = $options['db_passwd'];
    $subsite['db_host']   = $options['db_host'];
    $subsite['valid'] = TRUE;
    $valid_count ++;
    drush_log(dt('Subsite @site loaded.', array('@site' => $subsite['name'])));
  }

  // Check number of valid subsites
  if ($valid_count == 0) {
    drush_log(dt('Could not load any subsite.'), 'warning');
    return;
  }

  $task->options['subsites'] = $subsites;
  drush_log(dt('Finished preparing subsite(s).'));
}

/**
 * Remember to remove the subsite links.
 */
function drush_hosting_subsite_post_hosting_delete_task() {
  // @XXX Since Aegir never seems to delete the nodes, we can ignore this.
  // @TODO If subsites have been deleted, we should reverify parents.
}

/**
 * Re-verify parent site after disabling.
 */
function drush_hosting_subsite_post_hosting_disable_task() {
  // @TODO
}

/**
 * Re-verify parent site after enabling.
 */
function drush_hosting_subsite_post_hosting_enable_task() {
  // @TODO
}

/**
 * Re-verify parent site after migrating.
 * @XXX Migrate vs. Verify: will 'verify' flood parent with 'verify'? This is in terms of server load and/or frontend usability.
 */
function drush_hosting_subsite_post_provision_migrate() {
  // @TODO
}
