<?php
/**
 * Subsite management.
 *
 * @author Taihao Zhang (zhang.taihao@gmail.com)
 * @license GPLv2. See LICENSE.
 */

/**
 * Page callback.
 * Subsites listing page.
 */
function hosting_subsite_subsites($node) {
  modalframe_parent_js();
  $base_link = 'node/'.$node->nid.'/subsites';
  $output = '';

  // Look for parent sites
  $result = db_query("SELECT n.nid, n.title name, l.path
      FROM {hosting_subsite_link} l JOIN {node} n ON l.parent = n.nid
      WHERE subsite = %d", $node->nid);
  $parent_list = array();
  while ($site = db_fetch_object($result)) {
    $parent_list[] = l($site->name, 'node/'.$site->nid) . '/' . $site->path;
  }
  // Site is already a subsite. Forbid creation of subsites.
  if (count($parent_list) > 0) {
    $output .= '<p>'.t('This site is already subsite as:').'</p>';
    $output .= theme('item_list', $parent_list);
    $output .= '<p>'.t('Subsites cannot be nested. Remove this subsite from all parent sites to link other sites as subsites.').'</p>';
    return $output;
  }

  // Fetch subsites
  $result = db_query("SELECT n.nid, n.title name, l.path
      FROM {hosting_subsite_link} l JOIN {node} n ON l.subsite = n.nid
      WHERE parent = %d", $node->nid);

  // Build table
  $headers = array(t('Edit'), t('Path'), t('Subsite'), t('Unlink'));
  $rows = array();
  while ($site = db_fetch_object($result)) {
    $rows[] = array(
      _hosting_subsite_button(t('Edit'),
          $base_link . '/' . $site->nid . '/edit',
          t('Configure relationship to this subsite.'),
          'hosting-subsite-edit-button'),
      $site->path,
      _hosting_subsite_goto_link($site->name, $site->nid),
      _hosting_subsite_button(t('Unlink'),
          $base_link . '/' . $site->nid . '/unlink',
          t('Configure relationship to this subsite.'),
          'hosting-subsite-unlink-button'),
    );
  }
  $output .= theme('table', $headers, $rows, array('id' => 'hosting-subsite-subsites'));

  return $output;
}

/**
 * Form button using hosting styles.
 */
function _hosting_subsite_button($label, $link, $tooltip, $class) {
  return l($label, $link, array(
      'attributes' => array(
        'title' => $tooltip,
        'class' => 'hosting-button-enabled '.$class,
      )
  ));
}

/**
 * Link for navigating to subsite node.
 */
function _hosting_subsite_goto_link($name, $nid) {
  return l($name, 'node/'.$nid, array('attributes' => array(
      'title' => t('Go to subsite info page.'),
  )));
}

/**
 * Subsite link edit form.
 */
function hosting_subsite_edit_form(&$form_state, $parent, $subsite) {
  $values = $form_state['values'];

  // Load values
  if (!isset($values['subsite'])) {
    $values['subsite'] = $subsite->title;
  }
  if (!isset($values['path'])) {
    $values['path'] = db_result(db_query("SELECT path FROM {hosting_subsite_link}
      WHERE parent = %d AND subsite = %d", $parent->nid, $subsite->nid));
  }

  // Autocomplete subsite textfield
  $form['subsite'] = array(
    '#title' => t('Subsite'),
    '#description' => t('Domain name of the subsite to link'),
    '#type' => 'textfield',
    '#autocomplete_path' => 'hosting_subsite/autocomplete/edit_subsite/'.$parent->nid,
    '#default_value' => $values['subsite'],
    '#size' => 40,
  );
  // Regular path textfield
  $form['path'] = array(
    '#title' => t('Path'),
    '#description' => t('URL path from the root to link the subsite under. Do not put a leading or trailing slash.'),
    '#type' => 'textfield',
    '#field_prefix' => '/',
    '#default_value' => $values['path'],
    '#size' => 20,
  );
  
  // Pass along form parameters
  $form_state['params']['parent'] = $parent->nid;
  $form_state['params']['subsite'] = $subsite->nid;
  $form_state['params']['subsite_name'] = $subsite->title;

  return $form;
}

/**
 * Subsite link edit form validator.
 */
function hosting_subsite_edit_form_validate(&$form, &$form_state) {
  static $safe = '%0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_.~';
  
  $values =& $form_state['values'];
  $params = $form_state['params'];

  // First check whether form values have changed
  if ($values['subsite'] == $params['subsite'] &&
      $values['path'] == $params['path']) {
    return;
  }
  
  // Non-empty subsite
  if (empty($values['subsite'])) {
    form_set_error('subsite', t('Please enter a subsite.'));
  }
  // Check subsite is indeed eligible (failsafe for autocomplete)
  elseif (empty(_hosting_subsite_list_eligible_subsites(
        $params['parent'], $values['subsite'], TRUE))) {
    form_set_error('subsite', t('Invalid subsite. Please enter the domain name of an enabled site that is neither an existing subsite under this current parent site nor a parent site itself.'));
  }

  // Non-empty path
  $path =& $values['path'];
  if (empty($path)) {
    form_set_error('path', t('Please enter a path.'));
  }
  // Check for leading or trailing slash
  elseif (substr($path, 0, 1) == '/' || substr($path, -1, 1) == '/') {
    $path = trim($path, '/');
    form_set_error('path', t('Please do not enter a leading or trailing slash. The slash(es) have been removed.'));
  }
  // Check for unsafe characters
  elseif (strspn($path, $safe) < strlen($path)) {
    form_set_error('path', t('Please enter only URL-safe characters for path.'));
  }
  // Check the cleaned path is not already specified
  elseif (db_result(db_query("SELECT * FROM {hosting_subsite_link}
            WHERE parent = %d AND path = '%s'", $params['parent'], $path))) {
    form_set_error('path', t('The specified path already exists under this parent site.'));
  }
}

/**
 * Subsite link edit form submit handler.
 */
function hosting_subsite_edit_form_submit($form, $form_state) {
  $values = $form_state['values'];
  $params = $form_state['params'];
  
  $parent = $params['parent'];
  $subsite = $params['subsite'];
  $dest = 'node/'.$parent.'/subsites';
  
  $site = hosting_get_site_by_url($values['subsite']);
  if (!$site) {
    drupal_set_message(t('Cannot save subsite.'), 'error');
    drupal_goto($dest);
  }
  
  $subsite_new = $site->nid;
  $path_new = $values['path'];
  
  // Execute query
  db_query("UPDATE {hosting_subsite_link} SET subsite = %d, path = '%s'
      WHERE parent = %d AND subsite = %d",
      $subsite_new, $path_new, $parent, $subsite);

  $affected = db_affected_rows();
  if ($affected == 0) {
    drupal_set_message(t('Nothing was saved.'), 'warning');
  }
  elseif ($affected == 1) {
    drupal_set_message(t('Sucessfully saved subsite.'));
  }
  else {
    drupal_set_message(t('More than one subsite has been affected.'), 'warning');
  }
  drupal_goto($dest);
}

/**
 * Autocomplete callback for subsite field of edit form.
 */
function _hosting_subsite_autocomplete_edit_subsite($string, $node) {
  $matches = _hosting_subsite_list_eligible_subsites($node->nid, $string);
  print drupal_to_js($matches);
  exit();
}

/**
 * Search helper function for listing eligible subsites of a given site ID.
 * @param $nid  Parent site ID to search under.
 * @param $filter  Name to filter site names by.
 * @param $exact  Whether to match exactly or merely as substring.
 * @return  Array of matching subsite names.
 */
function _hosting_subsite_list_eligible_subsites($nid, $filter = '', $exact = FALSE) {
  // Construct filter
  $filter_query = $filter == '' ? '' : 'AND n.title LIKE ' .
      ($exact ? "'%s'" : "'%%%s%%'");

  $matches = array();
  // Look for active sites that are not subsites of given node or parent sites
  $result = db_query_range("SELECT n.title
      FROM {node} n JOIN {hosting_site} s ON n.nid = s.nid
      WHERE n.type = 'site' AND
        -- Active site
        s.status <> %d AND
        -- Node is not...
        n.nid NOT IN (
          -- Subsite of given node
          SELECT subsite FROM {hosting_subsite_link} WHERE parent = %d UNION
          -- Parent site
          SELECT parent FROM {hosting_subsite_link})
        $filter_query", HOSTING_SITE_DELETED, $node->nid, $filter,
      0, 10);
  while ($site = db_result($result)) {
    $matches = $site;
  }

  return $matches;
}
