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
  $module_path = drupal_get_path('module', 'hosting_subsite');

  modalframe_parent_js();
  drupal_add_css($module_path.'/hosting_subsite.css');
  drupal_add_js($module_path.'/hosting_subsite.js');

  $base_link = 'node/'.$node->nid.'/subsites';
  $output = '';

  // Look for parent sites
  $result = db_query("SELECT n.nid, n.title name, l.path
      FROM {hosting_subsite} l JOIN {node} n ON l.parent = n.nid
      WHERE subsite = %d", $node->nid);
  $parent_list = array();
  while ($site = db_fetch_object($result)) {
    $parent_list[] = l($site->name, 'node/'.$site->nid) . '/' . $site->path;
  }
  // Site is already a subsite. Forbid creation of subsites.
  if (count($parent_list) > 0) {
    $output .= '<p>'.t('This site is a subsite under the following site locations:').'</p>';
    $output .= theme('item_list', $parent_list, NULL, 'ul', array('class' => 'hosting-subsite-parent-list'));
    $output .= '<p>'.t('Subsites cannot be nested. Remove this subsite from all parent sites to link other sites as subsites.').'</p>';
    return $output;
  }

  $output .= '<div class="hosting-subsite-notice">';
  $output .= t('To apply any changes you have made, !verify.', array(
    '!verify' => l(t('verify this site'), 'node/'.$node->nid.'/site_verify'),
  ));
  $output .= '</div>';

  // Fetch subsites
  $result = db_query("SELECT n.nid, n.title name, l.path
      FROM {hosting_subsite} l JOIN {node} n ON l.subsite = n.nid
      WHERE parent = %d", $node->nid);

  // Build table
  $rows = array();
  while ($site = db_fetch_object($result)) {
    $rows[] = array(
      _hosting_subsite_button(t('Edit'),
          $base_link . '/edit/' . $site->nid,
          t('Configure relationship to this subsite.'),
          'hosting-subsite-edit-button'),
      '/' . $site->path,
      _hosting_subsite_goto_link($site->name, $site->nid),
      _hosting_subsite_button(t('Unlink'),
          $base_link . '/remove/' . $site->nid,
          t('Remove this subsite from here.'),
          'hosting-subsite-remove-button'),
    );
  }
  if (!empty($rows)) {
    $headers = array(t('Edit'), t('Path'), t('Subsite'), t('Remove'));
    $output .= theme('table', $headers, $rows, array('id' => 'hosting-subsite-subsites'));
  } else {
    $output .= '<p>'.t('No subsites exist for this site. Add one below.').'</p>';
  }

  $output .= '<p>';
  $output .= _hosting_subsite_button(t('Add subsite'), $base_link . '/add',
      t('Add a subsite under a new path.'), 'hosting-subsite-add-button');
  $output .= '</p>';

  return $output;
}

/**
 * Form button using hosting styles.
 */
function _hosting_subsite_button($label, $link, $tooltip, $class) {
  return l($label, $link, array(
      'attributes' => array(
        'title' => $tooltip,
        'class' => 'hosting-subsite-button '.$class,
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
 * Subsite link edit form. If $subsite is not given, the form is treated as
 * for adding new links.
 */
function hosting_subsite_edit_form(&$form_state, $parent, $subsite = NULL) {
  $values = $form_state['values'];

  if (is_object($parent)) {
    $parent_nid = $parent->nid;
  }
  if (is_object($subsite)) {
    $subsite_name = $subsite->title;
    $subsite_nid = $subsite->nid;
  }

  // Load values
  if (!isset($values['subsite'])) {
    $values['subsite'] = $subsite_name;
  }
  if (!isset($values['path'])) {
    $values['path'] = db_result(db_query("SELECT path FROM {hosting_subsite}
      WHERE parent = %d AND subsite = %d", $parent_nid, $subsite_nid));
  }

  // Autocomplete subsite textfield
  $form['subsite'] = array(
    '#title' => t('Subsite'),
    '#required' => TRUE,
    '#description' => t('Domain name of the subsite to link'),
    '#type' => 'textfield',
    '#autocomplete_path' => 'hosting_subsite/autocomplete/edit_subsite/'.$parent_nid,
    '#default_value' => $values['subsite'],
    '#size' => 40,
  );
  // Regular path textfield
  $form['path'] = array(
    '#title' => t('Path'),
    '#required' => TRUE,
    '#description' => t('URL path from the root to link the subsite under. Do not put a leading or trailing slash.'),
    '#type' => 'textfield',
    '#field_prefix' => '/',
    '#default_value' => $values['path'],
    '#size' => 20,
  );

  // Explicit specify form handlers for use by add form
  $form['#validate'][] = 'hosting_subsite_edit_form_validate';
  $form['#submit'][] = 'hosting_subsite_edit_form_submit';

  // Pass along form parameters
  $form_state['params']['parent'] = $parent_nid;
  $form_state['params']['subsite'] = $subsite_nid;
  $form_state['params']['subsite_name'] = $subsite_name;
  $form_state['params']['path'] = $values['path'];

  $form = confirm_form($form, $subsite ? t('Edit subsite') : t('Add a subsite'), 'node/'.$parent_nid.'/subsites', '', t($subsite ? 'Save' : 'Add'));
  $form['#redirect'] = 'node/'.$parent_nid.'/subsites';

  drupal_add_js(drupal_get_path('module', 'hosting_subsite').'/hosting_subsite.js');

  return $form;
}

/**
 * Aliased clone of hosting_subsite_edit_form.
 */
function hosting_subsite_add_form(&$form_state, $parent) {
  return hosting_subsite_edit_form($form_state, $parent, NULL);
}

/**
 * Subsite link edit form validator.
 */
function hosting_subsite_edit_form_validate(&$form, &$form_state) {
  static $safe = '%0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_.~/';

  $values = $form_state['values'];
  $params = $form_state['params'];

  // First check whether form values have changed
  if ($values['subsite'] == $params['subsite_name'] &&
      $values['path'] == $params['path']) {
    return;
  }

  // Check subsite is indeed eligible (failsafe for autocomplete)
  if ($values['subsite'] != $params['subsite_name'] &&
      0 == count(_hosting_subsite_list_eligible_subsites(
        $params['parent'], $values['subsite'], TRUE))) {
    form_set_error('subsite', t('Invalid subsite. Please enter the domain name of a non-parent site that is not an existing subsite under this current parent site.'));
  }

  $path = $values['path'];
  $subsite_old = (int) $params['subsite'];
  // Check for leading or trailing slash
  if (substr($path, 0, 1) == '/' || substr($path, -1, 1) == '/') {
    form_set_error('path', t('Please do not enter a leading or trailing slash.'));
  }
  // Check for unsafe characters
  elseif (strspn($path, $safe) < strlen($path)) {
    form_set_error('path', t('Please enter only URL-safe characters for path.'));
  }
  // Check the cleaned path is not already specified
  // (Precondition satisfied by now: given path does not conflict with
  // current subsite.)
  elseif (db_result(db_query("SELECT * FROM {hosting_subsite}
              WHERE parent = %d AND subsite <> %d AND path = '%s'",
              $params['parent'], $subsite_old, $path))) {
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

  $site = hosting_get_site_by_url($values['subsite']);
  if (!$site) {
    drupal_set_message(t('Cannot save subsite.'), 'error');
    $form_state['no_redirect'] = TRUE;
    return;
  }

  $subsite_new = $site->nid;
  $path_new = $values['path'];

  // Execute query
  if ($subsite) {
    db_query("UPDATE {hosting_subsite} SET subsite = %d, path = '%s'
        WHERE parent = %d AND subsite = %d",
        $subsite_new, $path_new, $parent, $subsite);
  } else {
    db_query("INSERT INTO {hosting_subsite} (parent, subsite, path)
        VALUES (%d, %d, '%s')",
        $parent, $subsite_new, $path_new);
  }

  $affected = db_affected_rows();
  if ($affected == 0) {
    drupal_set_message(t('Nothing was saved.'), 'warning');
    $form_state['no_redirect'] = TRUE;
    return;
  }
  elseif ($affected == 1) {
    drupal_set_message(t('Sucessfully saved subsite.'));
  }
  else {
    drupal_set_message(t('More than one subsite has been affected.'), 'warning');
    $form_state['no_redirect'] = TRUE;
    return;
  }

  modalframe_close_dialog(array('action' => 'refresh'));
}

/**
 * Subsite remove form.
 */
function hosting_subsite_remove_form(&$form_state, $parent, $subsite) {
  $form_state['params']['parent'] = $parent->nid;
  $form_state['params']['subsite'] = $subsite->nid;
  $title = t('Are you sure you want to remove this subsite?');
  $description = '<h1>'.$title.'</h1>';
  $description .= '<p>'.t('The subsite itself will not be deleted.').'</p>';
  return confirm_form(array(), t('Are you sure you want to remove this subsite?'),
      'node/'.$parent->nid.'/subsites', $description, t('Remove'));
}

/**
 * Subsite remove form submit handler.
 */
function hosting_subsite_remove_form_submit(&$form, &$form_state) {
  $params = $form_state['params'];
  $form_state['redirect'] = 'node/'.$params['parent'].'/subsites';
  db_query("DELETE FROM {hosting_subsite} WHERE parent = %d AND subsite = %d",
      $params['parent'], $params['subsite']);
  drupal_set_message(t('The subsite has been removed.'));

  modalframe_close_dialog(array('action' => 'refresh'));
}

/**
 * Autocomplete callback for subsite field of edit form.
 */
function _hosting_subsite_autocomplete_edit_subsite($node) {
  $string = arg(4);
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
            SELECT subsite FROM {hosting_subsite} WHERE parent = %d UNION
            -- Parent site (or given node)
            SELECT parent FROM {hosting_subsite} UNION
            -- Given node
            SELECT %d) $filter_query
        ORDER BY n.title ASC",
      HOSTING_SITE_DELETED, $nid, $nid, $filter,
      0, 10);
  while ($site = db_result($result)) {
    $matches[$site] = $site;
  }

  return $matches;
}
