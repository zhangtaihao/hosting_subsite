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
          $base_link . '/edit/' . $site->nid,
          t('Configure relationship to this subsite.'),
          'hosting-subsite-edit-button'),
      $site->path,
      _hosting_subsite_goto_link($site->name, $site->nid),
      _hosting_subsite_button(t('Unlink'),
          $base_link . '/unlink/' . $site->nid,
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
function hosting_subsite_edit_form($form_state) {
  // @TODO Autocomplete subsite textfield
  $form['subsite'] = array(
    '#title' => t('Subsite'),
    '#type' => 'textfield',
    '#size' => 40,
  );
  // Regular path textfield
  $form['path'] = array(
    '#title' => t('Path'),
    '#type' => 'textfield',
    '#field_prefix' => '/',
    '#size' => 20,
  );

  return $form;
}

/**
 * Subsite link edit form validator.
 */
function hosting_subsite_edit_form_validate(&$form, $form_state) {
  // @TODO
  // Validate path complies with URL format
}

/**
 * Subsite link edit form submit handler.
 */
function hosting_subsite_edit_form_submit($form, $form_state) {
  // @TODO
}
