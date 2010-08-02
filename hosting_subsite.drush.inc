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
@TODO
Approximate format of injected code:

    Alias /%subsite:path% "%platform:publish_path%"
    <Location /%subsite:path%>
      SetEnv db_type  xxxxx
      SetEnv db_name  xxxxx
      SetEnv db_user  xxxxx
      SetEnv db_passwd  xxxxx
      SetEnv db_host  xxxxx
    </Location>
    <Directory "%platform:publish_path%">
      AllowOverride FileInfo
    </Directory>

See:
  drupal_urlencode()

To escape directory paths (yes, even for Apache!):
 1. Add slashes to double quotes
 2. Enclose string in double quotes to account for spaces
*/