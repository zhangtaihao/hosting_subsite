/**
 * Helper scripts to prepare manage links.
 *
 * @author Taihao Zhang (zhang.taihao@gmail.com)
 * @license GPLv2. See LICENSE.
 */

$(document).ready(function() {
  // Set up parent window buttons
  $('.hosting-subsite-button').click(function() {
    $(document).data('hostingOpenModalFrame', true)
    var options = {
      url : '/hosting/js' + $(this).attr('href'),
      draggable : true,
      width : 600,
      height : 150,
      onSubmit : function(args) {
        if (args !== null && typeof(args) == 'object') {
          if (args['action'] == 'refresh') {
            location.reload();
          }
        }
      }
    }
    Drupal.modalFrame.open(options);
    return false;
  });
  
  // Set up child window buttons
  $('#hosting-subsite-edit-form a').click(function() {
    if (parent.Drupal.modalFrame.isOpen) {
      setTimeout(function() { parent.Drupal.modalFrame.close({}, {}); }, 1);
      return false;
    }
  });
});