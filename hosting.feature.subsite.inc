<?php
/**
 * Hosting feature definition.
 *
 * @author Taihao Zhang (zhang.taihao@gmail.com)
 * @license GPLv2. See LICENSE.
 */

/**
 * Implementation of hook_hosting_feature.
 */
function hosting_subsite_hosting_feature() {
  $features['subsite'] = array(
    'title' => t('Subsites'),
    'description' => t('Link a site to a path under another site.'),
    'status' => HOSTING_FEATURE_DISABLED,
    'module' => 'hosting_subsite',
  );
  return $features;
}
